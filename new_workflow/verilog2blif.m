function [] = verilog2blif(filename)
    if (ispc)
        bin_name = 'abc.exe';
%     elseif (ismac)
%         bin_name = 'mcxyzn.mac';
%     elseif (isunix)
%         bin_name = 'mcxyzn.linux';
    else
        fprintf('Could not find the appropriate binary \n');
    end
    filepath = which(bin_name);
    system_command_string = sprintf('%s -c "read_verilog %s.v; write %s.blif"',bin_name,filename,filename);

    [status] = system(system_command_string);
end
