%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%Image Printing
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function printImage(mat, activityTimeIndex, arborId, outDir, scaleFlag, figTitle)
   global VIEW_FIGS;
   global GRAY_SC;
   global WRITE_FIGS;
   
   if(VIEW_FIGS)
      figure;
   else
      figure('Visible', 'off');
   end
   if (scaleFlag < 0)
      scaleMax = max(max(mat(:)), abs(min(mat(:))));
      scale = [-scaleMax, scaleMax];
   else
      scale = [-scaleFlag, scaleFlag]
   end
   imagesc(mat, scale);
   %Find max/min of mat, and set scale equal to that
   if(GRAY_SC)
      colormap(gray);
   else
      colormap(cm());
   end

   colorbar;
   title([figTitle, ' - time: ', num2str(activityTimeIndex), ' arbor: ', num2str(arborId)]);
   if(WRITE_FIGS)
      print_movie_filename = [outDir, figTitle, '_', num2str(activityTimeIndex), '_', num2str(arborId), '.jpg'];
      print(print_movie_filename);
   end
end
