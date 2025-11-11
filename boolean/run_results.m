clear
variable_number = 4;
test_values = linspace(0,65535,65536);
min_terms_bin = dec2bin(test_values,16);

min_terms = cell(65536,1);
for i = 1:65536
   intermediate = [];
   for j = 1:16
       if min_terms_bin(i,j) == '1'
           intermediate = [intermediate j-1];
       end
   end
   min_terms{i,1} = intermediate;
end

PI = cell(65536,1);
for i = 1:65535
    PI{i,1} = Q_M(variable_number,min_terms{i,1},[]);
    %print_function(variable_number,PI{i,1})
end
%save('PI.mat','min_terms','min_terms_bin','PI')
%print_function(variable_number,essential_PI)