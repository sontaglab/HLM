function [logic_out] = bin2logic(bin_in)
    for i = 1:length(bin_in)
        if (bin_in(i) == '1')
            logic_out(i,1) = 1;
        else
            logic_out(i,1) = 0; 
        end
    end
end