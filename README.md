# eb

My programming language. Compiles to llvm and stuff.

	// structures!
	struct Cat {
		age: Int,
		hp: I32,
		calico: Bool
	}
	fn new_cat(hp: I32): Cat {
		Cat(age=0, hp=hp, calico=false)
	}
	
	// functions with optional parameters!
	fn do_thing(a: Int, b: Int, [c: Int = 0]): Int {
		a + b * c
	}
	
	// function overloading!
	fn do_thing(a: Int, b: Bool): Int {
		// implicit returns and expression ifs!
		if b { a * 2 } else { a / 2 }
	}
	
	fn main(): I32 {
		// type inference!
		x := do_thing(1, 2) + do_thing(3, 4, c=5) + do_thing(6, true)
		cat := new_cat(x)
	
		// control flow!
		while cat.hp > 0 {
			cat.hp -= 1
			x += 2
		}
	
		return x - 108
	}
