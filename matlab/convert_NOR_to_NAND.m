clear
load('PI.mat')
vector_4 = getcondvects(4);

PI_double = cell(65536,1);
nb_terms = cell(65536,1);
abs_PI_double = cell(65536,1);
sum_terms = cell(65536,1);

for i = 1:length(PI)
    terms = PI{i,1};
    [row,col] = size(terms);
    nb_terms{i,1} = zeros(1,row);
    for j = 1:row
        for k = 1:4
            if terms(j,k) == '1'
               PI_double{i,1}(j,k) = 1;
               nb_terms{i}(j) = nb_terms{i}(j) + 1;
            elseif terms(j,k) == '0'
               PI_double{i,1}(j,k) = -1;
               nb_terms{i}(j) = nb_terms{i}(j) + 1;
            else
               PI_double{i,1}(j,k) = 0;
            end
        end
    end
    abs_PI_double{i,1} = abs(PI_double{i,1});
    abs_PI_double{i,1} = (abs_PI_double{i,1}~=0);
    sum_terms{i,1} = sum(abs_PI_double{i,1},2);
end


binary_representation('

for i = 1:length(PI)
    
end