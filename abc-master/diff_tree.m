function [nodes2,node_names2] = diff_tree(nodes,node_names,node_a,node_b)
    [~,~,saved_a] = get_subtree(nodes,node_names,node_a);
    [~,~,saved_b] = get_subtree(nodes,node_names,node_b);
    
    check = 0;
    
    nodes2 = nodes;
    node_names2 = node_names;

    [val] = intersect(saved_a,saved_b,'sorted');
    if isempty(val)
        check = 1;
    end
    val = setdiff(saved_a,val);
    if check == 0
        val = [val, node_b];
    end
    
    for i = 1:length(nodes)
        if ~ismember(length(nodes)-i+1,val)
            nodes2(length(nodes)-i+1) =[];
            node_names2(length(nodes)-i+1) =[];
            for j = 1:length(nodes2)
               if (nodes2(j) >=  length(nodes)-i+1)
                  nodes2(j) = nodes2(j) - 1; 
               end
            end
        end
    end
    
    vector_count = zeros(1,length(nodes2));
    for k = 1:length(nodes2)
        if nodes2(k) ~= 0
            vector_count(1,nodes2(k)) = vector_count(1,nodes2(k)) + 1;
        end
    end
    
   for i = 1:length(nodes2)
      if (vector_count(i) == 0 && ~contains(node_names2{1,i}(1),'x'))
           node_names2{1,i} = 'x_{new}';
      end
   end
end