function [] = print_individual_cells(nb_decimal,all_perms,subplot_nb,total_offending_terms)
    load('summary.mat')
    %load('PI.mat')
    load('nb_norgates.mat')
    load('permuted_terms.mat')
    load('maxgates.mat')
    load('p_repeats.mat')
    
    %tip_names = strings;
    
    offending = 0;
    if summary(nb_decimal+1,2) == 2
       nb_maxgate = maxgates_2(find(inputs_2 == summary(dec_rep(nb_decimal+1,1)+1,3),1));
       fprintf('The minimum number of gates to implement in a single cell is: %i \n',nb_maxgate)
       fprintf('The NAND P-equivalent decimal input representation is %i \n',summary(dec_rep(nb_decimal+1,1)+1,3))
       remaining_terms = 1:4;
       remaining_terms(permuted_terms(dec_rep(nb_decimal+1,1)+1,:)==1) = [];
       remaining_terms = remaining_terms(all_perms{1,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
       fprintf('With the following input permutation: %i %i \n',remaining_terms-1)
       fprintf('The NOR gate representation is given by \n')
       print_graphs(summary(nb_decimal+1,2),find(inputs_2 == summary(dec_rep(nb_decimal+1,1)+1,3),1),remaining_terms,subplot_nb,offending)

    elseif summary(nb_decimal+1,2)== 3
       nb_maxgate = maxgates_3(find(inputs_3 == summary(dec_rep(nb_decimal+1,1)+1,3),1));
       fprintf('The minimum number of gates to implement in a single cell is: %i \n',nb_maxgate)
       fprintf('The NAND P-equivalent decimal input representation is %i \n',summary(dec_rep(nb_decimal+1,1)+1,3))
       remaining_terms = 1:4;
       remaining_terms(permuted_terms(dec_rep(nb_decimal+1,1)+1,:)==1) = [];
       remaining_terms = remaining_terms(all_perms{2,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
       fprintf('With the following input permutation: %i %i %i\n',remaining_terms-1)
       fprintf('The NOR gate representation is given by \n')
       print_graphs(summary(nb_decimal+1,2),find(inputs_3 == summary(dec_rep(nb_decimal+1,1)+1,3),1),remaining_terms,subplot_nb,offending)
    elseif summary(nb_decimal+1,2)== 4
       nb_maxgate = maxgates_4(find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1,1),1));
       fprintf('The minimum number of gates to implement in a single cell is: %i \n',nb_maxgate)
       fprintf('The NAND P-equivalent decimal input representation is %i \n',summary(dec_rep(nb_decimal+1,1)+1,1))
       remaining_terms = 1:4;
       remaining_terms(permuted_terms(dec_rep(nb_decimal+1,1)+1,:)==1) = [];
       remaining_terms = remaining_terms(all_perms{3,1}(summary(dec_rep(nb_decimal+1,1)+1,4),:));
       fprintf('With the following input permutation: %i %i %i %i\n',remaining_terms-1)
       fprintf('The NOR gate representation is given by \n')
       if (nb_maxgate > 7) && (total_offending_terms > 1)
          offending = 1;
          print_graphs(summary(nb_decimal+1,2),find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1),1),remaining_terms,subplot_nb,offending)
       elseif (nb_maxgate > 7)
          offending = 1;
          print_graphs(summary(nb_decimal+1,2),find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1),1),remaining_terms,subplot_nb,offending)
       else
          print_graphs(summary(nb_decimal+1,2),find(inputs_4 == summary(dec_rep(nb_decimal+1,1)+1),1),remaining_terms,subplot_nb,offending)
       end
    else
       nb_maxgate = 0;
       fprintf('A representation is not needed \n') 
    end
    title(sprintf('Cell #%i, NOR gates: %i',subplot_nb,nb_maxgate))
    set(gca,'xtick',[],'ytick',[])
    box on
%    ax = gca;
%     outerpos = ax.OuterPosition;
%     ti = ax.TightInset; 
%     left = outerpos(1) + ti(1);
%     bottom = outerpos(2) + ti(2);
%     ax_width = outerpos(3) - ti(1) - ti(3);
%     ax_height = outerpos(4) - ti(2) - ti(4);
%     ax.Position = [left bottom ax_width ax_height];
end