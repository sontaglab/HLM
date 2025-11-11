function [] = print_function(variable_number,essential_PI)
    english_alphabet = 'abcdefghijklmnopqrstuvwxyz';
    fprintf('f(');
    for i = 1:variable_number
        fprintf(english_alphabet(i));
        if (i~=variable_number)
            fprintf(', ');
        end
    end
    fprintf(') = ');
    [row_epi, col_epi] = size(essential_PI);
    for i = 1:row_epi
        for j = 1:variable_number
            if (essential_PI{i,2}(j) == '1')
                fprintf(english_alphabet(j));
            elseif (essential_PI{i,2}(j) == '0')
                fprintf(english_alphabet(j));
                fprintf('*');
            end
        end
        if (i~=row_epi)
            fprintf(' + ');
        else
            fprintf('\n');
        end
    end
end

