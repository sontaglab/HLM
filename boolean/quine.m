function [essential_PI] = quine(variable_number,minterm)
    dont_care = [];

    md = sort([minterm, dont_care]); 
    group = cell(variable_number+1); 
    group_binary = cell(variable_number+1); 
    group_PI_flag = cell(variable_number+1); 
    PI = {}; 
    
    for i = 1:length(md)
        binary_value = dec2bin(md(i), variable_number);
        temp1 = []; 
        temp2 = linspace(0, 0, variable_number); 
        for j = 1:variable_number
            temp1 = [temp1, str2num(binary_value(j))];
        end
        which_group = pdist([temp1;temp2],'hamming') * variable_number + 1;
        group{which_group,1} = [group{which_group,1}; md(i)];
        group_binary{which_group,1} = [group_binary{which_group,1}; binary_value]; 
    end


    for i = 1:variable_number 
        final_group_number = variable_number+1-i; 
        for j = 1:final_group_number+1
            [row_j, col_j] = size(group{j,i});
            for k = 1:row_j            
                group_PI_flag{j,i}(k,1) = 1;
            end
        end    
        for j = 1:final_group_number
            [row_j, col_j] = size(group{j,i});
            for k = 1:row_j
                temp1 = [];
                for b = 1:variable_number
                    temp1 = [temp1, str2num(group_binary{j,i}(k,b))];
                end
                [row_k, col_k] = size(group{j+1,i});
                for m = 1:row_k
                    temp2 = [];
                    for b = 1:variable_number 
                        temp2 = [temp2, str2num(group_binary{j+1,i}(m,b))];
                    end
                    if (pdist([temp1;temp2],'hamming') * variable_number == 1)
                        group_PI_flag{j,i}(k,1) = 0;
                        group_PI_flag{j+1,i}(m,1) = 0;
                        next_step_element = sort([group{j,i}(k,:), group{j+1,i}(m,:)]);
                        next_step_binary = '';
                        for n = 1:variable_number
                            if temp1(n) ~= temp2(n)
                                next_step_binary(n) = '9';
                            else
                                next_step_binary(n) = num2str(temp1(n));
                            end
                        end
                        quit = 0;
                        [row_m, col_m] = size(group{j,i+1});
                        for n = 1:row_m
                            if (pdist([next_step_element;group{j,i+1}(n,:)],'hamming') == 0)
                                quit = 1;
                            end
                        end
                        if (quit==0)
                            group{j,i+1} = [group{j,i+1}; next_step_element];
                            group_binary{j,i+1} = [group_binary{j,i+1}; next_step_binary];
                        end
                    end
                end
            end
        end
        for j = 1:final_group_number+1
            [row_j, col_j] = size(group{j,i});
            for k = 1:row_j
                if(group_PI_flag{j,i}(k,1) == 1)
                    [row_p, col_p] = size(PI);
                    PI{row_p+1,1} = group{j,i}(k,:);
                    PI{row_p+1,2} = group_binary{j,i}(k,:);
                end
            end
        end
    end
    
    
    [row_p, col_p] = size(PI);
    PI_chart = zeros([row_p, length(minterm)]);
    for i = 1:row_p
        for j = 1:length(PI{i,1})
            for k = 1:length(minterm)
                if (PI{i,1}(1,j) == minterm(k))
                    PI_chart(i,k) = 1;
                end
            end
        end
    end
    essential_PI = {};
    row_epi_record = []; 
    
    for i = 1:length(minterm)
        [row_e, col_e] = find(PI_chart(:,i));
        if (length(row_e) == 1)
            [row_pass, col_pass] = find(row_epi_record,row_e);
            if (isempty(row_pass))
                row_epi_record = [row_epi_record; row_e];
                [row_ep, col_ep] = size(essential_PI);
                essential_PI{row_ep+1,1} = PI{row_e,1};
                essential_PI{row_ep+1,2} = PI{row_e,2};
            end
        end
    end
    
    [row_ep, col_ep] = size(essential_PI);
    for i = 1:row_ep
        for j = 1:length(essential_PI{i,1})
            delete = find(minterm==essential_PI{i,1}(1,j));
            if (length(delete)==1)
                PI_chart(:,delete) = 0;
            end
        end
    end
    [row_pi, col_pi] = find(PI_chart);
    while (~isempty(row_pi))
        [row_pid, col_pid] = size(PI);
        PI_be_dominant = linspace(0,0,row_pid);
        for i = 1:length(unique(row_pi))
            for j = 1:length(unique(row_pi))
                if (i~=j)
                    row_pi_sort = unique(row_pi);
                    be_dominant = find((PI_chart(row_pi_sort(i),:) > PI_chart(row_pi_sort(j),:)) == (PI_chart(row_pi_sort(i),:) ~= PI_chart(row_pi_sort(j),:)));
                    if (length(be_dominant)==length(minterm))
                        PI_be_dominant(row_pi_sort(j)) = 1;
                    end
                end
            end
        end
        if (~isempty(find(PI_be_dominant,1)))
            temp = find(PI_be_dominant,1);
            for i = 1:length(temp)
                PI_chart(temp(i),:) = 0;
            end
        end
        row_epi_record = []; 
        for i = 1:length(minterm)
            [row_e, col_e] = find(PI_chart(:,i));
            if (length(row_e) == 1)
                [row_pass, col_pass] = find(row_epi_record,row_e);
                if (isempty(row_pass))
                    row_epi_record = [row_epi_record; row_e];
                    [row_ep, col_ep] = size(essential_PI);
                    essential_PI{row_ep+1,1} = PI{row_e,1};
                    essential_PI{row_ep+1,2} = PI{row_e,2};
                end
            end
        end
        [row_ep, col_ep] = size(essential_PI);
        for i = 1:row_ep
            for j = 1:length(essential_PI{i,1})
                delete = find(minterm==essential_PI{i,1}(1,j));
                if (length(delete)==1)
                    PI_chart(:,delete) = 0;
                end
            end
        end
        [row_ep, col_ep] = size(essential_PI);
        for i = 1:row_ep
            for j = 1:length(essential_PI{i,1})
                delete = find(minterm==essential_PI{i,1}(1,j));
                if (length(delete)==1)
                    PI_chart(:,delete) = 0;
                end
            end
        end
        [row_pi, col_pi] = find(PI_chart);
    end
end