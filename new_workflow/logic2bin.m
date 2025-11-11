function [bin_out] = logic2bin(logic_in)
    for i = 1:length(logic_in)
        if (logic_in(i) == 1)
            bin_out(i) = '1';
        else
            bin_out(i) = '0'; 
        end
    end
end