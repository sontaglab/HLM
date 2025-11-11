clear
load('PI.mat')

nb_norgates = cell(65536,1);

for i = 1:65536
   intermediate = [];
   for j = 1:size(PI{i,1})
       if length(PI{i,1}(j,:)) == 4
           n = 0;
           q = 0;
           for k = 1:4
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

for i = 1:65536
   max_norgates{i,1} = max(nb_norgates{i,1});
   if isempty(max_norgates{i,1})
       max_norgates{i,1} = 0;
   elseif max_norgates{i,1} < 0           
       max_norgates{i,1} = 0;
   end
   N_terms(i) = length(nb_norgates{i,1});
end

max_norgates = cell2mat(max_norgates);

N = hist(max_norgates,linspace(0,10,10));
N_cum = cumsum(N)/655.36;
N_1_total = sum(N_terms==1);