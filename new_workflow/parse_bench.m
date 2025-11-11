function [vector,vector_names] = parse_bench(name)
str = fileread(sprintf('%s.bench',name));
exp = '(?<=INPUT.)\w+';
inputs = regexp(str,exp,'match');

exp = '(?<=OUTPUT.)\w+';
output = regexp(str,exp,'match');

split_str = strsplit(str, '\n');

exp = '^\S*.*?=';
counter = 1;
counter2 = 1;
for i = 1:length(split_str)
     match_result = regexp(split_str{1,i},exp,'match');
     if ~isempty(match_result)
        match_result2 = regexp(match_result{1,1},'^\S*','match');
        gates{1,counter} = match_result2{1,1};
        type(1,counter) = 0;
        
        match_result3 = regexp(split_str{1,i},'(?<=AND.)\w+','match');
        if ~isempty(match_result3)
            match_result4 = regexp(split_str{1,i},'(?<=, )\w+','match');
            and_gates{1,counter2} = match_result3{1,1};
            and_gates{2,counter2} = match_result4{1,1};
            type(1,counter) = 1;
            counter2 = counter2 + 1;
        else
            match_result5 = regexp(split_str{1,i},'(?<=NOT.)\w+','match');
            not_gates{1,counter-counter2+1} = match_result5{1,1};
        end
        
        counter = counter + 1;
     end
end
% inputs
% gates
% type
% and_gates
% not_gates

counter = 1;
counter2 = 0;
current = output{1,1};
curr_pos = 1;
vector_names{1,1} = current;
vector = [0];

while (curr_pos <= length(vector_names))
    check = 0;
    for i = 1:length(gates)
       if strcmp(gates{1,i},current)
           if (type(i) == 1)
               vector(counter+1) = curr_pos;
               vector(counter+2) = curr_pos;
               vector_names{1,counter+1} = and_gates{1,sum(type(1:i))};
               vector_names{1,counter+2} = and_gates{2,sum(type(1:i))};
               counter = counter + 2; 
               curr_pos = curr_pos + 1;
               current = vector_names{1,curr_pos};
               counter2 = counter2 +1;
               check = 1;
           else
              vector(counter+1) = curr_pos;
              vector_names{1,counter+1} = not_gates{1,i-sum(type(1:i))};
              counter = counter + 1;
              curr_pos = curr_pos + 1;
              current = vector_names{1,curr_pos};
              counter2 = counter2 +1;
              check = 1;
           end
       end
    end
    if (check == 0)
        curr_pos = curr_pos + 1;
        if ~(length(vector_names) < curr_pos)          
            current = vector_names{1,curr_pos};
        end
    end
end

vector_count = zeros(1,length(vector));
for i = 2:length(vector)
   vector_count(1,vector(i)) = vector_count(1,vector(i)) + 1;
end
and_vector = (vector_count == 2);

min = 0;
maxi = sum(and_vector);
count = 0;

% close all
% title('AND2/NOT')
% treeplot_cust(vector,vector_names,0)
% warning off

while any(and_vector == 1) && (count < maxi)
    and_vector = (vector_count == 2);
    i = min + 1;
    while (i <= length(vector))
        if (vector_count(i) == 2)
            fprintf('Parent node: %i \n',i)
            fprintf('Children nodes:')
            bool = zeros(2,1);
            pos = zeros(2,1);
            counter = 0;
            for j = 1:length(vector)
               if (vector(j) == i)
                   fprintf('%i ',j)
                   counter = counter + 1;
                   pos(counter) = j;
                   if (vector_count(j) == 1)
                       fprintf('(NOT) ');
                       bool(counter) = 1;
                   end
               end
            end
            fprintf('\n')
%             vector
%             vector_count
%             pos
            if (bool(1) == 0)
                for k = 1:length(vector)
                    if vector(k) >= pos(1)
                        vector(k) = vector(k) + 1;
                    end
                end
                vector = [vector(1:pos(1)) pos(1)+1 vector(pos(1)+1:end)];
                vector_names = {vector_names{1,1:pos(1)-1} 'new_nX_' vector_names{1,pos(1):end}};
                vector(pos(1)+1) = pos(1);
                %vector_names(pos(1):pos(1)+1) = {vector_names{1,pos(1)+1} vector_names{1,pos(1)}};
                vector_count = zeros(1,length(vector));
                for k = 2:length(vector)
                   vector_count(1,vector(k)) = vector_count(1,vector(k)) + 1;
                end
                pos(2) = pos(2) + 1;
            else
                vector(pos(1)) =[];
                vector_names(pos(1)) = [];
                pos_next = find(vector == pos(1));
                vector(pos_next) = i;
                pos(2) = pos(2) - 1;
                for k = pos(1):length(vector)
                    if vector(k) >= pos(1)
                        vector(k) = vector(k) - 1;
                    end
                end
                vector_count = zeros(1,length(vector));
                for k = 2:length(vector)
                   vector_count(1,vector(k)) = vector_count(1,vector(k)) + 1;
                end
            end
%             vector
%             vector_count
%             pos
%             figure
%             treeplot_cust(vector,vector_names,0)
%             warning off
            if (bool(2) == 0)
                for k = 1:length(vector)
                    if vector(k) >= pos(2)
                        vector(k) = vector(k) + 1;
                    end
                end
                vector = [vector(1:pos(2)) pos(2)+1 vector(pos(2)+1:end)];
                vector_names = {vector_names{1,1:pos(2)-1} 'new_nX_' vector_names{1,pos(2):end}};
                vector(pos(2)+1) = pos(2);
                %vector_names(pos(2):pos(2)+1) = {vector_names{1,pos(2)+1} vector_names{1,pos(2)}};
                vector_count = zeros(1,length(vector));
                for k = 2:length(vector)
                   vector_count(1,vector(k)) = vector_count(1,vector(k)) + 1;
                end
            else
                vector(pos(2)) =[];
                vector_names(pos(2)) = [];
                pos_next = find(vector == pos(2));
                vector(pos_next) = i;
                for k = 1:length(vector)
                    if vector(k) >= pos(2)
                        vector(k) = vector(k) - 1;
                    end
                end
                vector_count = zeros(1,length(vector));
                for k = 2:length(vector)
                   vector_count(1,vector(k)) = vector_count(1,vector(k)) + 1;
                end
            end
            min = i ;
            count = count + 1;
%             figure
%             treeplot_cust(vector,vector_names,0)
%             warning off
            break;
        end
        i = i + 1;
    end
end
close all
figure
title('NOR2')
treeplot_cust(vector,vector_names,0)
warning off

%print(sprintf('%s',name),'-dpng','-r300')
end