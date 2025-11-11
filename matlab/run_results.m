clear
tic
N = 10000;
variable_number = 5;
test_values = randi([0 round(2^(2^(4.5)))-1],N,1);
min_terms_bin = dec2bin(test_values,32);

min_terms = cell(65536,1);
for i = 1:N
   intermediate = [];
   for j = 1:(2^5)
       if min_terms_bin(i,j) == '1'
           intermediate = [intermediate j-1];
       end
   end
   min_terms{i,1} = intermediate;
end
tic
PI = cell(N,1);
for i = 1:N
    i
    PI{i,1} = Q_M(variable_number,min_terms{i,1},[]);
    %print_function(variable_number,PI{i,1})
end
toc


nb_norgates = cell(N,1);

for i = 1:N
   intermediate = [];
   for j = 1:size(PI{i,1})
       if length(PI{i,1}(j,:)) == 5
           n = 0;
           q = 0;
           for k = 1:5
              if (PI{i,1}(j,k) == '0') || (PI{i,1}(j,k) == '1')
                  n = n + 1;
              end
              if (PI{i,1}(j,k) == '0')
                  q = q + 1;
              end
           end
           intermediate = [intermediate 3*(n-1)-q];
       end
   end
   nb_norgates{i,1} = intermediate;
end

for i = 1:N
   max_norgates(i) = max(nb_norgates{i,1});
   if isempty(max_norgates(i))
       max_norgates(i) = 0;
   elseif max_norgates(i) < 0           
       max_norgates(i) = 0;
   end
end

% N_y = hist(max_norgates,linspace(0,15,16));
% N_cum = cumsum(N_y)/N*100;
% N_1_total = sum(N_terms==1);

save('PI_5inputs_rand10.mat','min_terms','min_terms_bin','PI','nb_norgates','max_norgates')
%print_function(variable_number,essential_PI)