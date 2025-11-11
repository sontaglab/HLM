clear
[nodes,node_names] = parse_bench('ttt_3');

nodes_count = zeros(1,length(nodes));
for k = 2:length(nodes)
   nodes_count(1,nodes(k)) = nodes_count(1,nodes(k)) + 1;
end

clf
subplot(1,2,1)
title('NOR2')
treeplot_cust(nodes,node_names,0)
warning off

for i = 1:length(nodes)
    for j = 1:length(nodes)
        if j > i && j > 2
            [nodes2,node_names2] = diff_tree(nodes,node_names,i,j);
            [bin_nb, gate_nb, input_nb] = evaluate_tree(nodes2,node_names2);
            hex_nb = bin2hex(bin_nb);
            if input_nb == 4
                bin2dec(bin_nb)
                gate_nb
            end
        end
    end
end

% [nodes2,node_names2] = diff_tree(nodes,node_names,4,9);
% 
% subplot(1,2,2)
% title('NOR2')
% treeplot_cust(nodes2,node_names2,0)
% warning off
%             [bin_nb, gate_nb, input_nb] = evaluate_tree(nodes2,node_names2);
