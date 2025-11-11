clear

fid = fopen('P3-Status.txt');
check = 0;
counter = 0;
while (check == 0)
    line_ex = fgetl(fid);
    counter = counter + 1;
    cell{counter,1} = line_ex;
    if line_ex == -1
       check = 1;
       cell{end,1} = [];
    end
end

relevant_lines = zeros(length(cell),2);
for i = 1:length(cell)
    if ~(isempty(strfind(cell{i,1},'-----------')))
       relevant_lines(i) = 1; 
    end
end
counter = 0;
for i = 1:length(cell)
    if rem(counter,3) == 1
        relevant(i,2) = 1;
    elseif rem(counter,3) == 2
        relevant(i,2) = 2;
    else
        relevant(i,2) = 0;
    end
    if (relevant_lines(i) == 1)
        counter = counter + 1; 
    end
end
for i = 1:length(cell)
    if (relevant(i,2)==2)
        relevant(i-2,2) = 2;
        relevant(i-1,2) = 2;
    end
end

counter = 1;
counter2 = 1;
for i = 1:length(cell)
    if (relevant(i,2)==1)
        graphs{counter,counter2} = cell{i,1};
        counter2 = counter2 + 1;
    elseif ((relevant(i,2)==2)&&(relevant(i-1,2)==1))
        counter = counter + 1;
        counter2 = 1;
    end
end

save('graphs_3inputs.mat','graphs')