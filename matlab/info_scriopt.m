clear
load('PI.mat')
load('nb_norgates')
load('summary.mat')
load('maxgates.mat')
nb_decimal= 27026
dec2bin(nb_decimal,16)
fprintf('The NOR gate requirements are: ') 
for i = 1:size(PI{nb_decimal+1,1},1)
    fprintf('%i ',nb_norgates{nb_decimal+1,1}(i))
end
fprintf('\n')
nb_maxgate = maxgates_4(find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1,1),1));
fprintf('The minimum number of gates to implement in a single cell is: %i \n',nb_maxgate)