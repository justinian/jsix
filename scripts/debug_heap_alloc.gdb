das -enabled off
break heap_allocator.cpp:81
commands
silent
printf "+ %016lx %4d\n", block, length
continue
end
break heap_allocator.cpp:86
commands
silent
printf "+ %016lx %4d\n", block, length
continue
end
break heap_allocator.cpp:120
commands
silent
printf "- %016lx\n", p
continue
end
break heap_allocator.cpp:140
commands
silent
printf "> %016lx %4d %4d\n", p, old_length, new_length
continue
end
set logging file heap_allocs.txt
set logging overwrite on
set logging enabled on
continue
