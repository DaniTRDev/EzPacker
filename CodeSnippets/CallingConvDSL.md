```dsl
calling_convention x86_64_sysv {
    // ------------------------------------------------------------------------
    // Frame & Stack Architecture
    // ------------------------------------------------------------------------
    stack {
        ALIGN(16)
        GROWTH(DOWN)
        CLEANUP(caller)
        SHADOW_SPACE(0)
        FRAME(GPR:rbp)
        STACK(GPR:rsp)
    };

    
    CALLER(GPR:rbx, GPR:rsp, GPR:rbp, GPR:r12, GPR:r13, GPR:r14, GPR:r15);
    CALLEE(GPR:rax, GPR:rcx, GPR:rdx, GPR:rsi, GPR:rdi, GPR:r8, GPR:r9, GPR:r10, GPR:r11);

    // ------------------------------------------------------------------------
    // Phase 1: Classification Rules
    // ------------------------------------------------------------------------
    classify {
        TYPES(i1, i8, i16, i32, i64, ptr) >> INTEGER;
        TYPES(f32, f64)                  >> FLOAT;

        aggregates {
            // Evaluated top-to-bottom
            IF_NON_TRIVIAL         => MEMORY;
            IF_UNALIGNED           => MEMORY;
            IF_SIZE_GT(64)           => MEMORY;

            // Eightbyte slicing & merge precedence for <= 64 byte structs
            SLICE(8);
            PRECEDENCE(MEMORY, INTEGER, FLOAT);
            POLICY(ALL_OR_NOTHING);
        };
    };

    // ------------------------------------------------------------------------
    // Phase 2: Argument Passing
    // ------------------------------------------------------------------------
    arguments {
        assign INTEGER {
            SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9);
            REST(STACK(8)); // Alignment.
        };

        assign FLOAT {
            SEQ(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7);
            REST(STACK(8));
        };

        assign MEMORY {
            STACK(8);
        };
    };

    // ------------------------------------------------------------------------
    // Phase 3: Returns & Struct Return (SRET)
    // ------------------------------------------------------------------------
    returns {
        sret {
            PTR(GPR:rdi);
            CONSUMES_ARG(true);
            RET_REG(GPR:rax);
        };

        assign INTEGER {
            SEQ(GPR:rax, GPR:rdx);
        };

        assign FLOAT {
            SEQ(FPR:xmm0, FPR:xmm1);
        };

        assign MEMORY {
            STACK(8);
        };
    };
}

```

```dsl
calling_convention x86_64_windows {
    // ------------------------------------------------------------------------
    // Frame & Stack Architecture
    // ------------------------------------------------------------------------
    stack {
        ALIGN(16);
        GROWTH(DOWN);
        CLEANUP(caller);
        SHADOW_SPACE(32);
        FRAME(GPR:rbp);
        STACK(GPR:rsp);
    };

    CALLER(GPR:rax, GPR:rcx, GPR:rdx, GPR:r8, GPR:r9, GPR:r10, GPR:r11);
    CALLEE(GPR:rbx, GPR:rsi, GPR:rdi, GPR:rbp, GPR:rsp, GPR:r12, GPR:r13, GPR:r14, GPR:r15);

    // ------------------------------------------------------------------------
    // Phase 1: Classification Rules
    // ------------------------------------------------------------------------
    classify {
        TYPES(i1, i8, i16, i32, i64, ptr) >> INTEGER;
        TYPES(f32, f64)                  >> FLOAT;

        aggregates {
            // Aggregates matching 1, 2, 4, or 8 bytes are passed as integers; everything else by reference
            IF_SIZE_IN(1, 2, 4, 8) => INTEGER;
            DEFAULT                => BY_REF; // ByRef != memory -> Copy on stack and pass a ptr to callee.
        };
    };

    // ------------------------------------------------------------------------
    // Phase 2: Argument Passing
    // ------------------------------------------------------------------------
    arguments {
        // Paired slots: Slot 0 = rcx/xmm0, Slot 1 = rdx/xmm1, Slot 2 = r8/xmm2, Slot 3 = r9/xmm3
        assign INTEGER {
            PAIRED(GPR:rcx, GPR:rdx, GPR:r8, GPR:r9);
            REST(STACK(8));
        };

        assign FLOAT {
            PAIRED(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3);
            REST(STACK(8));
        };

        assign BY_REF {
            PAIRED(GPR:rcx, GPR:rdx, GPR:r8, GPR:r9);
            REST(STACK(8));
        };
    };

    // ------------------------------------------------------------------------
    // Phase 3: Returns & Struct Return (SRET)
    // ------------------------------------------------------------------------
    returns {
        sret {
            PTR(GPR:rcx);
            CONSUMES_ARG(true);
            RET_REG(GPR:rax);
        };

        assign INTEGER {
            SEQ(GPR:rax);
        };

        assign FLOAT {
            SEQ(FPR:xmm0);
        };

        assign BY_REF {
            PTR(GPR:rax);
        };
    };
}

```