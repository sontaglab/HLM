clear
load('summary.mat')
test_values = linspace(0,65535,65536);
min_terms_bin = dec2bin(test_values,16);
min_terms_bin = (min_terms_bin == '1');

vector_2 = getcondvects(2);
vector_3 = getcondvects(3);
vector_4 = getcondvects(4);
all_perms = unique(perms([1 2 3 4]),'rows');

relevant_terms = cell(65536,1);

p_equivalent = zeros(65536,1);
p_index = zeros(65536,1);

parfor i = 1:65536
    relevant_terms{i,1} = vector_4(min_terms_bin(i,:)==1,:);
    min_dec = zeros(length(all_perms),1);
    for j = 1:length(all_perms)
        permuted = relevant_terms{i,1}(:,all_perms(j,:));
        bin_rep = '0000000000000000';
        for k = 1:size(permuted,1)
            [~,index] = ismember(permuted(k,:),vector_4,'rows');
            bin_rep(index) = '1';
        end
        min_dec(j) = bin2dec(bin_rep);
    end
    [p_equivalent(i),p_index(i)] = min(min_dec);
end

index_A= find(~ismember(unique(p_equivalent),inputs_4,'rows'));
unique_p_eq = unique(p_equivalent);
diff_elements = unique_p_eq(index_A);
permutations = {[1],[2],[3],[4]}'; %
%permutations = flipud(permutations);
nb_terms = ones(65536,1)*4;
permuted_terms = zeros(65536,4);

for i = 1:65536
     relevant_terms{i,1} = vector_4(min_terms_bin(i,:)==1,:);
     if size(relevant_terms{i,1},1) > 0
         for j = 1:length(permutations)
            test_term = relevant_terms{i,1};
            for k = 1:length(permutations{j,1})
                test_term(:,permutations{j,1}(k)) = 1 - test_term(:,permutations{j,1}(k));
            end
            test_term = sortrows(test_term);
            if (all(all(test_term == relevant_terms{i,1})))
               nb_terms(i) = nb_terms(i) - 1;
               permuted_terms(i,j) = 1;
            end
         end
     end
end
%save('

%N = hist(nb_terms(diff_elements+1),[0 1 2 3 4])

decimal = zeros(65536,1);
all_perms_2 = unique(perms([1 2]),'rows');
all_perms_3 = unique(perms([1 2 3]),'rows');

for i = 1:65536
    if nb_terms(i) == 2
       terms = relevant_terms{i,1}(:,permuted_terms(i,:)==0);
       perms_vect = zeros(2,1);
       for j = 1:size(all_perms_2,1)
           bin_rep = '0000';
           for k = 1:size(terms,1)
                [~,index] = ismember(terms(k,all_perms_2(j,:)),vector_2,'rows');
                bin_rep(index) = '1';
           end 
           perms_vect(j) = bin2dec(bin_rep);
       end
       [decimal(i),p_index(i)] = min(perms_vect);
    elseif nb_terms(i) == 3
       terms = relevant_terms{i,1}(:,permuted_terms(i,:)==0);
       perms_vect = zeros(6,1);
       for j = 1:size(all_perms_3,1)
           bin_rep = '00000000';
           for k = 1:size(terms,1)
                [~,index] = ismember(terms(k,all_perms_3(j,:)),vector_3,'rows');
                bin_rep(index) = '1';
           end
           perms_vect(j) = bin2dec(bin_rep);
       end
       [decimal(i),p_index(i)] = min(perms_vect);
    end
end

summary = [p_equivalent,nb_terms,decimal,p_index];
%save('summary.mat','summary')