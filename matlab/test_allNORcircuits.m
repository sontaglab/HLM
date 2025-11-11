clear
warning off
load('summary.mat')
load('maxgates.mat')
load('permuted_terms')
%nb_decimal = randi([0 65535])
QS_nb = -ones(65536,1);
nb_gates = zeros(65536,1);
QS_nb(1) = 0; QS_nb(65536) = 0;
parfor i = 2:65535
    subplot_nb = [1:6];
    nb_decimal = i - 1;
    all_perms_2 = unique(perms([1 2]),'rows');
    all_perms_3 = unique(perms([1 2 3]),'rows');
    all_perms_4 = unique(perms([1 2 3 4]),'rows');
    all_perms = {all_perms_2;all_perms_3;all_perms_4};
    remaining_terms = 1:4;
    remaining_terms(permuted_terms(dec_rep(nb_decimal+1,1)+1,:)==1) = [];

    nb_inputs = summary(nb_decimal+1,2);
    if nb_inputs == 1
        QS_nb(i) = 0;
        nb_gates(i) = 1;
    elseif nb_inputs == 2
        remaining_terms = remaining_terms(all_perms{1,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
        nb_gates(i) = maxgates_2(find(inputs_2 == summary(dec_rep(nb_decimal+1,1)+1,3)));
        QS_nb(i) = print_graphs(summary(nb_decimal+1,2),find(inputs_2 == summary(dec_rep(nb_decimal+1,1)+1,3)),remaining_terms,subplot_nb,0);
    elseif nb_inputs == 3
        remaining_terms = remaining_terms(all_perms{2,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
        nb_gates(i) = maxgates_3(find(inputs_3 == summary(dec_rep(nb_decimal+1,1)+1,3)));
        QS_nb(i) = print_graphs(summary(nb_decimal+1,2),find(inputs_3 == summary(dec_rep(nb_decimal+1,1)+1,3)),remaining_terms,subplot_nb,0);
    else
        remaining_terms = remaining_terms(all_perms{3,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
        nb_gates(i) = maxgates_4(find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1,1)));
        QS_nb(i) = print_graphs(summary(nb_decimal+1,2),find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1,1)),remaining_terms,subplot_nb,0);
    end
end

save('QS_nb_N8.mat','QS_nb','nb_gates')

load ('nb_norgates.mat')
Nq71 = zeros(65536,1);
for i = 1:65536
   sum_i(i) = sum(nb_norgates{i,1} > 8); 
   if (sum_i(i) < 1)  || (QS_nb(i) == 1)
      Nq71(i) = 1; 
   end
   if (sum_i(i) == 1) && (length(nb_norgates{i,1})==1)
      Nq71(i) = 1; 
   end
end
sum(Nq71)/65536

load ('nb_norgates.mat')
Nq71 = zeros(65536,1);
for i = 1:65536
   sum_i(i) = sum(nb_norgates{i,1} > 7); 
   if (sum_i(i) < 4)  || (QS_nb(i) == 1) || (QS_nb(i) == 2)
      Nq71(i) = 1; 
   end
  if (sum_i(i) == 1) && (length(nb_norgates{i,1})==1)
      Nq71(i) = 1; 
   end
end
sum(Nq71)/65536
% 
figure('Renderer', 'painters', 'Position', [100 100 1000 405])
clf
subplot(1,2,1)
histogram(nb_gates,linspace(0.5,13.5,14))
[N,edges] = histcounts(nb_gates,linspace(0.5,14.5,15));
N = N/(sum(N));
N = cumsum(N);
xlabel('Minimum number of required NOR2 gates','FontWeight','Bold')
ylabel('Number of Boolean functions','FontWeight','Bold')
xticks(0:15)
grid on
subplot(1,2,2)
plot(1:14,N,'k','LineWidth',1.5)
hold on
scatter(1:14,N,'k')
xlabel('Minimum number of required NOR2 gates','FontWeight','Bold')
ylabel('Cumulative fraction of realizable Boolean functions','FontWeight','Bold')
grid on
xticks(0:15)
print('figure1.png','-dpng','-r300')