function [] = write_verilog(filename,bin)
    fileID = fopen(sprintf('%s.v',filename),'w');
    
    nb_inputs = log2(length(bin));
    vect = getcondvects(nb_inputs);
    bin2logic(bin)
    vect = vect(bin2logic(bin) == 1,:);
    
    input_txt = 'x_1';
    for i = 2:nb_inputs
        add_txt = sprintf('x_%i',i);
        input_txt = sprintf('%s,%s',input_txt,add_txt);
    end
    
    assign_txt = '';
    for i = 1:size(vect,1)
        if i > 1
            assign_txt = strcat(assign_txt ,' |');
        end
        assign_txt = strcat(assign_txt ,' (');
        for j = 1:nb_inputs
            if j > 1
               assign_txt = strcat(assign_txt ,' &'); 
            end
            if (vect(i,j) == 0)
                add_txt = sprintf(' ~x_%i',j);
            else
                add_txt = sprintf(' x_%i',j);
            end
            assign_txt = strcat(assign_txt,add_txt); 
        end
        assign_txt = strcat(assign_txt,' )');
    end
    % Writing the required code in the file
    fprintf(fileID,sprintf('module rca2 (%s,s);\n',input_txt));
    fprintf(fileID,'//-------------Input Ports Declarations-----------------------------\n');
    fprintf(fileID,sprintf('input %s;\n',input_txt));
    fprintf(fileID,'//-------------Output Ports Declarations-----------------------------\n');
    fprintf(fileID,'output s;\n');
    fprintf(fileID,'//-------------Logic-----------------------------------------------\n');
    fprintf(fileID,sprintf('assign s = %s;\n',assign_txt));
    fprintf(fileID,'endmodule');
    
    fclose(fileID);
end
