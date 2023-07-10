das -enabled off

break heap_allocator.cpp:200
commands
silent
printf "n %016lx %d\n", m_end, current
continue
end

break heap_allocator.cpp:206
commands
silent
printf "N %016lx %d\n", m_end, order
continue
end

break heap_allocator::register_free_block
commands
silent
printf "F %016lx %d\n", block, order
continue
end

break heap_allocator.cpp:118
commands
silent
printf "f %016lx %d\n", block, info->order
continue
end

break heap_allocator.cpp:241
commands
silent
printf "S %016lx %d\n", block, order
continue
end

break heap_allocator.cpp:158
commands
silent
printf "P %016lx %d\n", block, order
continue
end

break heap_allocator.cpp:180
commands
silent
printf "p %016lx %d\n", block, order
continue
end

break heap_allocator.cpp:182
commands
silent
printf "M %016lx %016lx %d\n", block, buddy, order
continue
end

set logging file buddy_allocs.txt
set logging overwrite on
set logging enabled on
continue
