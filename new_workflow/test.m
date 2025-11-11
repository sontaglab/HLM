name = 'haha';

bin = logic2bin(randi([0 1],16,1));

write_verilog(name,bin);
launch_simulation(name);
[vector,vector_names] = parse_bench(name);