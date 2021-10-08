function [z, f, e, n] = parse_image_filename(filename)

    delimiters = {'_', '.', 'z', 'f', 'e', 'i'};
    str_split = strsplit(filename, delimiters);
    
    z = str2double(str_split{3});
    f = str2double(str_split{4});
    e = str2double(str_split{5});
    n = str2double(str_split{6});

end
