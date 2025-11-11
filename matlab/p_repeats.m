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
p_index = cell(65536,1);

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
    for j = 1:length(all_perms)
        if  min_dec(j) == min(min_dec)
            p_index{i} = [p_index{i} j];
        end
    end
end

save('p_repeats.mat','p_index')