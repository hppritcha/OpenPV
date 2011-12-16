clip_ids = [1:50]; %% [7:17,21:22,30:31]; %%
clip_name = cell(length(clip_ids),1);
for i_clip = 1 : length(clip_name)                                                                                                                
  clip_name{i_clip} = num2str(clip_ids(i_clip), "%3.3i");
endfor

home_path = ...
    [filesep, "home", filesep, "garkenyon", filesep];
NEOVISION_DATASET_ID = "Heli"; %% "Tower"; %%  "Tail"; %% 
neovision_dataset_id = tolower(NEOVISION_DATASET_ID); %% 
NEOVISION_DISTRIBUTION_ID = "Training"; %% "Challenge"; %% "Formative"; %% 
neovision_distribution_id = tolower(NEOVISION_DISTRIBUTION_ID); %% 
repo_path = [filesep, "mnt", filesep, "data1", filesep, "repo", filesep];
program_path = [repo_path, ...
		"neovision-programs-petavision", filesep, ...
		NEOVISION_DATASET_ID, filesep, ...
		NEOVISION_DISTRIBUTION_ID, filesep]; %% 		  
ObjectType = "Car"; %% "Cyclist"; %%  
pvp_edge_filter = "canny";
pvp_frame_skip = 1;
pvp_frame_offset = 1;
num_ODD_kernels = 3;  %% 
clip_log_dir = [program_path, ...
		"log", filesep, ObjectType, filesep];
clip_log_pathname = [clip_log_dir, "log.txt"];
if exist(clip_log_pathname, "file")
  clip_log_struct = struct;
  clip_log_fid = fopen(clip_log_pathname, "r");
  clip_log_struct.tot_unread = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_rejected = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_clips = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_DoG = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_canny = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_cropped = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_mean = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_std = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_border_artifact_top = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_border_artifact_bottom = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_border_artifact_left = str2num(fgets(clip_log_fid));
  clip_log_struct.tot_border_artifact_right = str2num(fgets(clip_log_fid));
  clip_log_struct.ave_original_size = str2num(fgets(clip_log_fid));
  clip_log_struct.ave_cropped_size = str2num(fgets(clip_log_fid));
  clip_log_struct.std_original_size = str2num(fgets(clip_log_fid));
  clip_log_struct.std_cropped_size = str2num(fgets(clip_log_fid));
  clip_log_struct.max_cropped_size = str2num(fgets(clip_log_fid));
  clip_log_struct.min_cropped_size = str2num(fgets(clip_log_fid));
  fclose(clip_log_fid);
  patch_size = ...
      fix(clip_log_struct.ave_cropped_size); %% + clip_log_struct.std_original_size);
  std_patch_size = clip_log_struct.std_cropped_size;
  max_patch_size = clip_log_struct.max_cropped_size;
  min_patch_size = clip_log_struct.min_cropped_size;
else
  patch_size = [64, 64]; %%[128, 128];
  std_patch_size = [0, 0];
  max_patch_size = patch_size;
  min_patch_size = patch_size;
endif %% exist(clip_log_pathname)
disp(["patch_size = ", num2str(patch_size)]);
disp(["std_patch_size = ", num2str(std_patch_size)]);
pvp_layer = 7;  %% 
training_flag = 1;
num_procs = 24;  %% 

for i_clip = 1 : length(clip_name)
  disp(clip_name{i_clip});

  clip_path = [program_path, ...
	       pvp_edge_filter, filesep, ...
	       clip_name{i_clip}, filesep]; %% 
  pvp_path = [program_path, "activity", filesep, ...
	      clip_name{i_clip}, filesep, ObjectType, num2str(num_ODD_kernels), filesep, pvp_edge_filter, filesep];
  
  [num_frames, ...
   tot_frames, ...
   nnz_frames, ...
   tot_time, ...
   CSV_struct] = ...
      pvp_makeCSVFile2(NEOVISION_DATASET_ID, ...
		       NEOVISION_DISTRIBUTION_ID, ...
		       repo_path, ...
		       ObjectType, ...
		       pvp_edge_filter, ...
		       clip_name{i_clip}, ...
		       pvp_frame_skip, ...
		       pvp_frame_offset, ...
		       clip_path, ...
		       num_ODD_kernels, ...
		       patch_size, ...
		       std_patch_size, ...
		       max_patch_size, ...
		       min_patch_size, ...
		       pvp_layer, ...
		       pvp_path, ...
		       training_flag, ...
		       num_procs);
  csv_file = ...
      [program_path, "results", filesep, clip_name{i_clip}, filesep, ...
       ObjectType, num2str(num_ODD_kernels), filesep, pvp_edge_filter, filesep, ...
       NEOVISION_DATASET_ID, "_", clip_name{i_clip}, "_PetaVision_", ObjectType, "_002", ".csv"]  
  csv_repo = ...
      [repo_path, "neovision-results-", neovision_distribution_id, "-", neovision_dataset_id, clip_name{i_clip}, filesep]
  mkdir(csv_repo);
  copyfile(csv_file, csv_repo)
endfor


