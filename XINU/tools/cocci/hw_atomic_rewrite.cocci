                                   
                                                                                                                           
  
                                                                                 
                                                                              

@@ expression E, F; @@                                        

(
- hw_atomic_add(E, -1) + 1
+ os_atomic_dec_orig(E, relaxed)
|
- hw_atomic_add(E, -1)
+ os_atomic_dec(E, relaxed)
|
- hw_atomic_add(E, -F) + F
+ os_atomic_sub_orig(E, F, relaxed)
|
- hw_atomic_add(E, -F)
+ os_atomic_sub(E, F, relaxed)
|
- hw_atomic_add(E, 1) - 1
+ os_atomic_inc_orig(E, relaxed)
|
- hw_atomic_add(E, 1)
+ os_atomic_inc(E, relaxed)
|
- hw_atomic_add(E, F) - F
+ os_atomic_add_orig(E, F, relaxed)
|
- hw_atomic_add(E, F)
+ os_atomic_add(E, F, relaxed)
)

@@ expression E, F; @@                                        

(
- hw_atomic_sub(E, -1) - 1
+ os_atomic_inc_orig(E, relaxed)
|
- hw_atomic_sub(E, -1)
+ os_atomic_inc(E, relaxed)
|
- hw_atomic_sub(E, -F) - F
+ os_atomic_add_orig(E, F, relaxed)
|
- hw_atomic_sub(E, -F)
+ os_atomic_add(E, F, relaxed)
|
- hw_atomic_sub(E, 1) + 1
+ os_atomic_dec_orig(E, relaxed)
|
- hw_atomic_sub(E, 1)
+ os_atomic_dec(E, relaxed)
|
- hw_atomic_sub(E, F) + F
+ os_atomic_sub_orig(E, F, relaxed)
|
- hw_atomic_sub(E, F)
+ os_atomic_sub(E, F, relaxed)
)

@@ expression E, F; @@                                  

(
- hw_atomic_and(E, ~F)
+ os_atomic_andnot(E, F, relaxed)
|
- hw_atomic_and(E, F)
+ os_atomic_and(E, F, relaxed)
|
- hw_atomic_and_noret(E, ~F)
+ os_atomic_andnot(E, F, relaxed)
|
- hw_atomic_and_noret(E, F)
+ os_atomic_and(E, F, relaxed)
)

@@ expression E, F; @@                                

(
- hw_atomic_or(E, F)
+ os_atomic_or(E, F, relaxed)
|
- hw_atomic_or_noret(E, F)
+ os_atomic_or(E, F, relaxed)
)

@@ expression E, F, A; @@                        

(
- hw_compare_and_store(E, F, A)
+ os_atomic_cmpxchg(A, E, F, acq_rel)
)

               
