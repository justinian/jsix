das -enabled off
break frame_allocator.cpp:62
commands
silent
printf "+ %016lx %3d\n", *address, n
continue
end
break frame_allocator.cpp:95
commands
silent
printf "- %016lx %3d\n", address, count
continue
end

set logging file frame_allocs.txt
set logging overwrite on
set logging enabled on
continue
