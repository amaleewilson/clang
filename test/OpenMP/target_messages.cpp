// RUN: %clang_cc1 -verify -fopenmp -std=c++11 -o - %s
// RUN: not %clang_cc1 -fopenmp -std=c++11 -fopenmp-targets=aaa-bbb-ccc-ddd -o - %s 2>&1 | FileCheck %s

// RUN: %clang_cc1 -verify -fopenmp-simd -std=c++11 -o - %s
// CHECK: error: OpenMP target is invalid: 'aaa-bbb-ccc-ddd'
// RUN: not %clang_cc1 -fopenmp -std=c++11 -triple nvptx64-nvidia-cuda -o - %s 2>&1 | FileCheck --check-prefix CHECK-UNSUPPORTED-HOST-TARGET %s
// RUN: not %clang_cc1 -fopenmp -std=c++11 -triple nvptx-nvidia-cuda -o - %s 2>&1 | FileCheck --check-prefix CHECK-UNSUPPORTED-HOST-TARGET %s
// CHECK-UNSUPPORTED-HOST-TARGET: error: The target '{{nvptx64-nvidia-cuda|nvptx-nvidia-cuda}}' is not a supported OpenMP host target.
// RUN: not %clang_cc1 -fopenmp -std=c++11 -fopenmp-targets=hexagon-linux-gnu -o - %s 2>&1 | FileCheck --check-prefix CHECK-UNSUPPORTED-DEVICE-TARGET %s
// CHECK-UNSUPPORTED-DEVICE-TARGET: OpenMP target is invalid: 'hexagon-linux-gnu'

// RUN: not %clang_cc1 -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=powerpc64le-ibm-linux-gnu -emit-llvm %s -fopenmp-is-device -fopenmp-host-ir-file-path 1111.bc -o - 2>&1 | FileCheck --check-prefix NO-HOST-BC %s
// NO-HOST-BC: The provided host compiler IR file '1111.bc' is required to generate code for OpenMP target regions but cannot be found.

// RUN: %clang_cc1 -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=powerpc64le-ibm-linux-gnu -emit-llvm-bc %s -o %t-ppc-host.bc -DREGION_HOST
// RUN: not %clang_cc1 -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=powerpc64le-ibm-linux-gnu -emit-llvm %s -fopenmp-is-device -fopenmp-host-ir-file-path %t-ppc-host.bc -o - -DREGION_DEVICE 2>&1 | FileCheck %s --check-prefix NO-REGION
// NO-REGION: Offloading entry for target region is incorrect: either the address or the ID is invalid.
// NO-REGION-NOT: Offloading entry for target region is incorrect: either the address or the ID is invalid.

#if defined(REGION_HOST) || defined(REGION_DEVICE)
void foo() {
#ifdef REGION_HOST
#pragma omp target
  ;
#endif
#ifdef REGION_DEVICE
#pragma omp target
  ;
#endif
}
#pragma omp declare target to(foo)
void bar() {
#ifdef REGION_HOST
#pragma omp target
  ;
#endif
#ifdef REGION_DEVICE
#pragma omp target
  ;
#endif
}
#else
void foo() {
}

#pragma omp target // expected-error {{unexpected OpenMP directive '#pragma omp target'}}

int main(int argc, char **argv) {
  #pragma omp target { // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target ( // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target [ // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target ] // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target ) // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target } // expected-warning {{extra tokens at the end of '#pragma omp target' are ignored}}
  foo();
  #pragma omp target
  foo();
  // expected-warning@+1 {{extra tokens at the end of '#pragma omp target' are ignored}}
  #pragma omp target unknown()
  foo();
  L1:
    foo();
  #pragma omp target
  ;
  #pragma omp target
  {
    goto L1; // expected-error {{use of undeclared label 'L1'}}
    argc++;
  }

  for (int i = 0; i < 10; ++i) {
    switch(argc) {
     case (0):
      #pragma omp target
      {
        foo();
        break; // expected-error {{'break' statement not in loop or switch statement}}
        continue; // expected-error {{'continue' statement not in loop statement}}
      }
      default:
       break;
    }
  }

  goto L2; // expected-error {{use of undeclared label 'L2'}}
  #pragma omp target
  L2:
  foo();
  #pragma omp target
  {
    return 1; // expected-error {{cannot return from OpenMP region}}
  }

  [[]] // expected-error {{an attribute list cannot appear here}}
  #pragma omp target
  for (int n = 0; n < 100; ++n) {}

  return 0;
}
#endif
