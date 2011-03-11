function [amoeba_image_x, amoeba_image_y, amoeba_struct] = ...
      offsetAmoebaSegments(amoeba_struct, amoeba_image_x, amoeba_image_y)

  seg_list = 1 : length(amoeba_image_x);

  %% set location and fix boundaries (use mirror BCs)
  offset_x = 2 * ( rand(1) - 0.5 ) * ( fix(amoeba_struct.image_rect_size/2) - ...
				      amoeba_struct.outer_diameter );
  offset_y = 2 * ( rand(1) - 0.5 ) * ( fix(amoeba_struct.image_rect_size/2) - ...
				      amoeba_struct.outer_diameter );
  for i_seg = seg_list
    amoeba_segment_x = amoeba_image_x{i_seg};
    amoeba_segment_y = amoeba_image_y{i_seg};
    amoeba_segment_x = ...
        amoeba_segment_x + fix(amoeba_struct.image_rect_size/2) + offset_x;
    amoeba_segment_x = ...
        amoeba_segment_x .* ((amoeba_segment_x >= 1) & (amoeba_segment_x <= amoeba_struct.image_rect_size)) + ...
        (2 * amoeba_struct.image_rect_size - amoeba_segment_x ) .* (amoeba_segment_x > amoeba_struct.image_rect_size) + ...
        (1 - amoeba_segment_x) .* (amoeba_segment_x < 1);
    amoeba_segment_y = ...
        amoeba_segment_y + fix(amoeba_struct.image_rect_size/2) + offset_y;
    amoeba_segment_y = ...
        amoeba_segment_y .* ((amoeba_segment_y >= 1) & (amoeba_segment_y <= amoeba_struct.image_rect_size)) + ...
        (2 * amoeba_struct.image_rect_size - amoeba_segment_y ) .* (amoeba_segment_y > amoeba_struct.image_rect_size) + ...
        (1 - amoeba_segment_y) .* (amoeba_segment_y < 1);
    amoeba_image_x{i_seg} = amoeba_segment_x;
    amoeba_image_y{i_seg} = amoeba_segment_y;
  endfor
