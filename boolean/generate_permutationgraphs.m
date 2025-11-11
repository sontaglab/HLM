clear
close all
load('summary.mat')
load('PI.mat')
load('nb_norgates.mat')
load('permuted_terms.mat')
load('maxgates.mat')

%figure('Renderer', 'painters', 'Position', [100 100 1050 420])


all_perms_2 = unique(perms([1 2]),'rows');
all_perms_3 = unique(perms([1 2 3]),'rows');
all_perms_4 = unique(perms([1 2 3 4]),'rows');
all_perms = {all_perms_2;all_perms_3;all_perms_4};
%example %
% check = 0;
% while (check ==0)
%     nb_decimal = randi([0,65535]);%12051%38505;%
%     total_offending_terms = sum(nb_norgates{nb_decimal+1,1}>7);
%     if total_offending_terms < 1
%         check = 1;
%     end
% end

nb_decimal = 42964;
%nb_decimal = 42667;


%nb_decimal = 31398;
%nb_decimal = 24582;

%nb_decimal = 18822;

total_offending_terms = sum(nb_norgates{nb_decimal+1,1}>7);

counter = 1;
counter2 = 1;
for i = 1:size(PI{nb_decimal+1,1},1)
    if nb_norgates{nb_decimal+1,1}(i) > 7
        loc_offending(counter) = i;
        counter = counter + 1; 
    else
        loc_nonoffending(counter2) = i;
        counter2 = counter2 + 1; 
    end
end

if total_offending_terms == 2
    nb_QS = 1;
    bool_eq = zeros(1,4);
    for k = 1:4
        if PI{nb_decimal+1,1}(loc_offending(1),k) == PI{nb_decimal+1,1}(loc_offending(2),k)
            bool_eq(k) = 1;
        end
    end
    bool_eq_idx = [1,2];
elseif total_offending_terms == 3
    nb_QS = 2;
    C = nchoosek([1 2 3],2);
    for i = 1:size(C,1)
         bool_eq2 = zeros(1,4);
         for k = 1:4
             if PI{nb_decimal+1,1}(loc_offending(C(i,1)),k) == PI{nb_decimal+1,1}(loc_offending(C(i,2)),k)
                 bool_eq2(k) = 1;
             end
         end
         if sum(bool_eq2) == 2
            bool_eq = bool_eq2; 
            bool_eq_idx = [C(i,1),C(i,2)];
         end
    end
elseif total_offending_terms == 4
    nb_QS = 2;
    C = [1 2 3 4];
    counter = 1;
    for i = 1:size(C,1)
         bool_eq2 = zeros(4,1);
         for k = 1:4
             if PI{nb_decimal+1,1}(loc_offending(1),k) == PI{nb_decimal+1,1}(loc_offending(2),k)
                 bool_eq2(k) = 1;
             end
         end
         if sum(bool_eq2) == 2
            bool_eq(1,:) = bool_eq2; 
            bool_eq_idx(1,:) = [C(i,1),C(i,2)];
         end
    end
    for i = 1:size(C,1)
         bool_eq2 = zeros(4,1);
         for k = 1:4
             if PI{nb_decimal+1,1}(loc_offending(3),k) == PI{nb_decimal+1,1}(loc_offending(4),k)
                 bool_eq2(k) = 1;
             end
         end
         if sum(bool_eq2) == 2
            bool_eq(2,:) = bool_eq2; 
            bool_eq_idx(2,:) = [C(i,3),C(i,4)];
         end
    end
end

all_partitions = partitions(length(loc_nonoffending));
vector_4 = getcondvects(4);
for i = 1:size(PI{nb_decimal+1,1},1)
    binary_rep = '0000000000000000';
    for j = 1:size(vector_4,1)
        bin_nb = zeros(4,1);
        for k = 1:4
             if (PI{nb_decimal+1,1}(i,k) == '0') && (vector_4(j,k) == 0)
                 bin_nb(k) = 1;
             elseif (PI{nb_decimal+1,1}(i,k) == '1') && (vector_4(j,k) == 1)
                 bin_nb(k) = 1;
             elseif (PI{nb_decimal+1,1}(i,k) == '-')
                 bin_nb(k) = 1;
             end
        end
        if all(bin_nb)
           binary_rep(j) = 1 + '0'; 
        end
    end
    all_binary_rep(i,:) = binary_rep;
end

all_gates = cell(size(all_partitions,1),1);
all_gates_bin = cell(size(all_partitions,1),1);

for i = 1:size(all_partitions,1)
    for j = 1:size(all_partitions{i,1},2)
        sum_bin = all_binary_rep(loc_nonoffending(all_partitions{i,1}{1,j}(1)),:);
        for k = 2:size(all_partitions{i,1}{1,j},2)
            sum_bin2 = all_binary_rep(loc_nonoffending(all_partitions{i,1}{1,j}(k)),:);
            for l = 1:16
               if (sum_bin2(l) == '1')
                    sum_bin(l) = 1 + '0';
               end
            end
        end
        decimal_nb = bin2dec(sum_bin);
        if summary(decimal_nb+1,2) == 2
           nb_maxgate = maxgates_2(find(inputs_2 == summary(dec_rep(decimal_nb+1,1)+1,3),1));
        elseif summary(decimal_nb+1,2) == 3
           nb_maxgate = maxgates_3(find(inputs_3 == summary(dec_rep(decimal_nb+1,1)+1,3),1));
        elseif summary(decimal_nb+1,2) == 4
           nb_maxgate = maxgates_4(find(inputs_4 == summary(dec_rep(decimal_nb+1,1)+1,1),1));
        end
        all_gates{i,1} =[all_gates{i,1},nb_maxgate];
        all_gates_bin{i,1} = [all_gates_bin{i,1}; decimal_nb];
    end
end

best_gate = 100;
best_idx = 0;
for i = 1:size(all_gates,1)
    if all(all_gates{i,1}<8) && (sum(all_gates{i,1})<best_gate)
       best_gate = sum(all_gates{i,1});
       best_idx = i;
    end
end

for i = 1:size(all_gates,1)
    gate(i,1) = sum(all_gates{i,1});
    gate(i,2) = length(all_gates{i,1});
    gate(i,3) = gate(i,1)/gate(i,2);
    gate(i,4) = max(all_gates{i,1});
    gate(i,5) = min(all_gates{i,1});
end

figure('Renderer', 'painters', 'Position', [100 100 1050 300])
subplot(1,3,1)
for i = 1:length(gate(:,2))
    hold on
    if gate(i,4) > 7 
        scatter(gate(i,2),gate(i,3),'r','filled');
    else
        scatter(gate(i,2),gate(i,3),'g','filled');
    end
end
xlim([0.5 5.5])
ylim([4 11.5])
xlabel('# of cells','FontWeight','bold')
ylabel('Average # of gates per cell','FontWeight','bold')

subplot(1,3,2)
hold on
for i = 1:length(gate(:,2))
    hold on
    if gate(i,4) > 7 
        scatter(gate(i,2),gate(i,1),'r','filled');
    else
        scatter(gate(i,2),gate(i,1),'g','filled');
    end
end
xlim([0.5 5.5])
ylim([10 22.5])
xlabel('# of cells','FontWeight','bold')
ylabel('Total # of gates','FontWeight','bold')

subplot(1,3,3)
hold on
for i = 1:length(gate(:,2))
    hold on
    if gate(i,4) > 7 
        scatter(gate(i,2),gate(i,4)-gate(i,5),'r','filled');
    else
        scatter(gate(i,2),gate(i,4)-gate(i,5),'g','filled');
    end
end
xlim([0.5 5.5])
ylim([0 7.5])
xlabel('# of cells','FontWeight','bold')
ylabel('Range of gates #','FontWeight','bold')
%box on
%print('average_42964','-dpng','-r300')