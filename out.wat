(module
 (type $ref_array (array anyref))
 (type $lua_function (func (param (ref null $ref_array) (ref null $ref_array)) (result (ref null $ref_array))))
 (type $function (struct (field (ref null $lua_function)) (field (ref null $ref_array))))
 (type $string (array i8))
 (type $integer (struct (field i64)))
 (type $table (struct (field (ref null $ref_array))))
 (data $0 "a")
 (data $1 "b")
 (data $2 "asdad")
 (elem declare func $0)
 (export "start" (func $start))
 (func $0 (type $lua_function) (param $args (ref null $ref_array)) (param $upvalues (ref null $ref_array)) (result (ref null $ref_array))
  (return
   (ref.null none)
  )
 )
 (func $start (type $lua_function) (param $args (ref null $ref_array)) (param $upvalues (ref null $ref_array)) (result (ref null $ref_array))
  (local $2 anyref)
  (local $3 anyref)
  (local.set $2
   (struct.new $table
    (array.new_fixed $ref_array 4
     (array.new_data $string $0
      (i32.const 0)
      (i32.const 1)
     )
     (struct.new $integer
      (i64.const 1)
     )
     (array.new_data $string $1
      (i32.const 0)
      (i32.const 1)
     )
     (array.new_data $string $2
      (i32.const 0)
      (i32.const 5)
     )
    )
   )
  )
  (local.set $3
   (struct.new $function
    (ref.func $0)
    (ref.null none)
   )
  )
  (drop
   (call_ref $lua_function
    (array.new_fixed $ref_array 0)
    (struct.get $function 1
     (ref.cast (ref null $function)
      (local.get $3)
     )
    )
    (struct.get $function 0
     (ref.cast (ref null $function)
      (local.get $3)
     )
    )
   )
  )
  (return
   (ref.null none)
  )
 )
)
