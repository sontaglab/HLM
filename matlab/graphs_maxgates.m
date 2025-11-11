clear
load('summary.mat')

maxgates_2 = zeros(8,1);
maxgates_3 = zeros(68,1);
maxgates_4 = zeros(3904,1);

parfor i = 1:size(graphs_4inputs,1)
    check = 0;
    while check == 0
        for k = 4:100
            bool = zeros(size(graphs_4inputs,2),1);
            for j = 1:size(graphs_4inputs,2)
                if (strfind(graphs_4inputs{i,j},sprintf('g_%i',k)))
                    bool(j) = 1;
                end
            end
            if ~any(bool)
                check = 1;
                maxgates_4(i) = k - 4;
                break;
            end
        end
    end
end

parfor i = 1:size(graphs_3inputs,1)
    check = 0;
    while check == 0
        for k = 3:100
            bool = zeros(size(graphs_3inputs,2),1);
            for j = 1:size(graphs_3inputs,2)
                if (strfind(graphs_3inputs{i,j},sprintf('g_%i',k)))
                    bool(j) = 1;
                end
            end
            if ~any(bool)
                check = 1;
                maxgates_3(i) = k - 3;
                break;
            end
        end
    end
end

parfor i = 1:size(graphs_2inputs,1)
    check = 0;
    while check == 0
        for k = 2:100
            bool = zeros(size(graphs_2inputs,2),1);
            for j = 1:size(graphs_2inputs,2)
                if (strfind(graphs_2inputs{i,j},sprintf('g_%i',k)))
                    bool(j) = 1;
                end
            end
            if ~any(bool)
                check = 1;
                maxgates_2(i) = k - 2;
                break;
            end
        end
    end
end

save('maxgates.mat','maxgates_2','maxgates_3','maxgates_4')