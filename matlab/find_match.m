function [match, num, min_ex, max_ex] = find_match(data, low_limit, high_limit, mid_idx)

    match = (data > low_limit) == (data < high_limit);
    
    % this is where we find the group of matches that contains mid_idx
    measurements = regionprops(logical(match), 'Extrema');
    
    for jdx=1:numel(measurements)
        
        min_ex = floor(min(measurements(jdx).Extrema(:,1)));
        max_ex = ceil(max(measurements(jdx).Extrema(:,1)));
        
        if((mid_idx >= min_ex) && (mid_idx <= max_ex))
            num = max_ex - min_ex;
            break;
        end
            
    end
    
    match = zeros(size(match));
    match(min_ex:max_ex) = 1;

end