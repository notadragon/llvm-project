| seq | commit | non-test +/- | test +/- | files | depends |
|-----|--------|--------------|----------|-------|---------|
| 100 | 0100-upstream-bugfixes | +55/-8 | +297/-3 | 12 | -- |
| 1000 | 1000-p2900-base-basic | +425/-9 | +70/-0 | 15 | -- |
| 1020 | 1020-p2900-base-ast | +2446/-75 | +0/-0 | 35 | -- |
| 1040 | 1040-p2900-base-parse | +1347/-39 | +806/-0 | 23 | -- |
| 1060 | 1060-p2900-base-sema-core | +2902/-17 | +578/-0 | 27 | -- |
| 1070 | 1070-p2900-base-sema-predicate-scope | +311/-24 | +1293/-0 | 26 | -- |
| 1080 | 1080-p2900-base-sema-templates | +512/-7 | +870/-0 | 15 | -- |
| 1090 | 1090-p2900-base-sema-constexpr | +751/-25 | +11430/-0 | 42 | -- |
| 1100 | 1100-p2900-base-codegen | +1846/-7 | +164/-0 | 14 | -- |
| 1120 | 1120-p2900-base-serialization | +289/-5 | +0/-0 | 9 | -- |
| 1140 | 1140-p2900-base-driver | +127/-0 | +123/-0 | 8 | -- |
| 1160 | 1160-p2900-base-libcxx | +815/-1 | +1384/-3 | 27 | -- |
| 1170 | 1170-p2900-base-lambdas | +677/-74 | +3068/-0 | 73 | -- |
| 1180 | 1180-p2900-base-tests | +0/-0 | +6603/-0 | 137 | -- |
| 1190 | 1190-base-open-bug-watch | +0/-0 | +2147/-0 | 70 | 1180-p2900-base-tests |
| 2200 | 2200-libcontracts | +1819/-6 | +413/-0 | 35 | -- |
| 2400 | 2400-p3595 | +1731/-0 | +2809/-0 | 170 | -- |
| 2900 | 2900-umbrella-option | +6/-0 | +0/-0 | 1 | -- |
| 3000 | 3000-p3097 | +475/-7 | +2659/-0 | 65 | -- |
| 3300 | 3300-p3290 | +77/-9 | +757/-5 | 33 | -- |
| 3600 | 3600-p3099 | +8/-0 | +783/-0 | 25 | -- |
| 4000 | 4000-p3098 | +718/-9 | +2635/-0 | 85 | -- |
| 4200 | 4200-p3400-core | +1415/-6 | +4478/-0 | 101 | -- |
| 4230 | 4230-p3400-facet-allowed-semantics | +179/-0 | +478/-0 | 10 | 4200-p3400-core |
| 4240 | 4240-p3400-facet-identification | +68/-0 | +524/-0 | 10 | 4200-p3400-core |
| 4250 | 4250-p3400-facet-string-transforms | +0/-0 | +434/-0 | 5 | 4200-p3400-core |
| 4260 | 4260-p3400-facet-local-violation-handler | +221/-0 | +801/-0 | 12 | 4200-p3400-core |
| 4270 | 4270-p3400-bypass-rethrowing-local-handler | +702/-0 | +1041/-0 | 14 | 4200-p3400-core |
| 4280 | 4280-p3400-facet-queryable | +0/-0 | +212/-0 | 4 | 4200-p3400-core |
| 4600 | 4600-p4283 | +148/-6 | +1141/-0 | 35 | -- |
| 4650 | 4650-cross-feature-matrix | +0/-0 | +3463/-0 | 26 | 3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283 |
| 4800 | 4800-p4298 | +65/-0 | +801/-0 | 30 | -- |
| 5000 | 5000-p4299 | +37/-0 | +1817/-0 | 83 | -- |
| 5200 | 5200-p4301 | +8/-0 | +155/-0 | 8 | -- |
| 6000 | 6000-p3100-core | +1696/-64 | +2017/-0 | 86 | -- |
| 6030 | 6030-p3100-ubsan-runtime | +426/-1 | +408/-0 | 12 | 6000-p3100-core |
| 6040 | 6040-p3100-asan-runtime | +387/-13 | +859/-0 | 23 | 6000-p3100-core |
| 6050 | 6050-p3100-tsan-runtime | +151/-0 | +444/-0 | 12 | 6000-p3100-core |
| 6060 | 6060-p3100-msan-runtime | +199/-59 | +327/-0 | 14 | 6000-p3100-core |
| 6100 | 6100-p3100-check-vptr | +0/-0 | +811/-0 | 17 | 6000-p3100-core |
| 6110 | 6110-p3100-check-null-and-alignment | +1794/-54 | +11910/-0 | 115 | 6000-p3100-core |
| 6120 | 6120-p3100-check-object-size | +0/-0 | +63/-0 | 2 | 6000-p3100-core |
| 6130 | 6130-p3100-check-nonnull-attribute | +0/-0 | +62/-0 | 2 | 6000-p3100-core |
| 6140 | 6140-p3100-check-returns-nonnull-attribute | +0/-0 | +68/-0 | 2 | 6000-p3100-core |
| 6150 | 6150-p3100-check-pointer-overflow | +0/-0 | +262/-0 | 7 | 6000-p3100-core |
| 6170 | 6170-p3100-check-shift | +0/-0 | +187/-0 | 12 | 6000-p3100-core |
| 6180 | 6180-p3100-check-integer-divide | +0/-0 | +588/-0 | 31 | 6000-p3100-core |
| 6190 | 6190-p3100-check-signed-integer-overflow | +103/-0 | +255/-0 | 16 | 6000-p3100-core |
| 6200 | 6200-p3100-check-invalid-value | +97/-0 | +713/-0 | 20 | 6000-p3100-core |
| 6210 | 6210-p3100-check-float-cast-overflow | +0/-0 | +259/-0 | 14 | 6000-p3100-core |
| 6220 | 6220-p3100-check-bounds | +29/-0 | +350/-0 | 20 | 6000-p3100-core |
| 6230 | 6230-p3100-check-flow-off | +88/-1 | +631/-0 | 33 | 6000-p3100-core |
| 6240 | 6240-p3100-check-unreachable | +0/-0 | +20/-0 | 1 | 6000-p3100-core |
| 6250 | 6250-p3100-check-vla-bound | +0/-0 | +20/-0 | 1 | 6000-p3100-core |
| 6260 | 6260-p3100-check-builtin | +0/-0 | +20/-0 | 1 | 6000-p3100-core |
| 6270 | 6270-p3100-check-float-divide-by-zero | +0/-0 | +392/-0 | 14 | 6000-p3100-core |
| 6280 | 6280-p3100-check-implicit-conversion | +0/-0 | +27/-0 | 1 | 6000-p3100-core |
| 6300 | 6300-p3100-check-pure-virtual | +87/-7 | +737/-0 | 22 | 6000-p3100-core |
| 6310 | 6310-p3100-check-enum-cast | +0/-0 | +278/-0 | 17 | 6000-p3100-core |
| 6320 | 6320-p3100-check-coroutine-flow-off | +51/-3 | +168/-0 | 10 | 6000-p3100-core |
| 6330 | 6330-p3100-check-assume-attribute | +198/-8 | +613/-0 | 33 | 6000-p3100-core |
| 6340 | 6340-p3100-check-pointer-arithmetic | +33/-0 | +1407/-0 | 47 | 6000-p3100-core, 6110-p3100-check-null-and-alignment, 6220-p3100-check-bounds |
| 6350 | 6350-p3100-check-null-member-pointer | +68/-0 | +629/-0 | 21 | 6000-p3100-core, 6110-p3100-check-null-and-alignment |
| 6360 | 6360-p3100-check-placement-new | +87/-0 | +777/-0 | 25 | 6000-p3100-core, 6110-p3100-check-null-and-alignment |
| 6380 | 6380-p3100-check-noreturn | +76/-0 | +421/-0 | 14 | 6000-p3100-core |
| 6390 | 6390-p3100-check-handler-member | +80/-0 | +515/-0 | 15 | 6000-p3100-core |
| 6400 | 6400-p3100-check-base-init-mem-fun | +91/-0 | +524/-0 | 16 | 6000-p3100-core, 6390-p3100-check-handler-member |
| 6410 | 6410-p3100-check-assign-overlap | +162/-0 | +644/-0 | 15 | 6000-p3100-core, 6110-p3100-check-null-and-alignment |
| 6900 | 6900-cross-paper-options | +91/-0 | +79/-0 | 5 | 2900-umbrella-option, 3000-p3097, 3300-p3290, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283, 4800-p4298, 5000-p4299, 5200-p4301, 6000-p3100-core |
| 6990 | 6990-p3100-user-docs | +200/-0 | +0/-0 | 2 | 6000-p3100-core, 6060-p3100-msan-runtime, 6110-p3100-check-null-and-alignment, 6410-p3100-check-assign-overlap |
| 9900 | 9900-fork-metadata | +10228/-0 | +0/-0 | 153 | -- |
| 9999 | 9999-unknown | +0/-0 | +0/-0 | 0 | -- |
