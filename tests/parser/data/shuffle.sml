
fun printall words =
   let fun dumpall wl = print (String.concatWith " " wl);
   in (dumpall words;
       print "\n")
   end;

fun try_all _ []	= ()
  | try_all f l		= let fun foreach lhs (h::tl) = (f h (lhs@tl);
							 foreach (lhs @ [h]) tl)
				| foreach _  _		= ();
			  in foreach [] l
			  end;

fun with_word head w rest	= let val stub = head @ [w]
				  in (printall stub;
				      try_all (with_word stub) rest)
				  end;

print "@@START\n";
val _ = try_all (with_word []) words;
print "@@END\n";

OS.Process.exit 0;
