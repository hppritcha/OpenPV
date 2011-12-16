
clip_ids = [1:50]; %% [7:17,21:22,30:31];
clip_name = cell(length(clip_ids),1);
for i_clip = 1 : length(clip_name)
  clip_name{i_clip} = num2str(clip_ids(i_clip), "%3.3i");
endfor

global PVP_VERBOSE_FLAG
PVP_VERBOSE_FLAG = 0;

global pvp_home_path
global pvp_workspace_path
global pvp_mlab_path
global pvp_clique_path
pvp_home_path = ...
    [filesep, "home", filesep, "garkenyon", filesep];
pvp_workspace_path = ...
    [pvp_home_path, "workspace-indigo", filesep];
pvp_mlab_path = ...
    [pvp_workspace_path, "PetaVision", filesep, "mlab", filesep];
pvp_clique_path = ...
    [pvp_workspace_path, "Clique2", filesep];

NEOVISION_DATASET_ID = "Heli"; 
neovision_dataset_id = tolower(NEOVISION_DATASET_ID); %% 
NEOVISION_DISTRIBUTION_ID = "Training";
neovision_distribution_id = tolower(NEOVISION_DISTRIBUTION_ID); %% 
pvp_repo_path = ...
    [filesep, "mnt", filesep, "data1", filesep, "repo", filesep];
pvp_program_path = ...
    [pvp_repo_path, "neovision-programs-petavision", filesep, ...
     NEOVISION_DATASET_ID, filesep, ...
     NEOVISION_DISTRIBUTION_ID, filesep];
pvp_input_path = ...
    [pvp_clique_path, "input", filesep, ...
     NEOVISION_DATASET_ID, filesep, ...
     NEOVISION_DISTRIBUTION_ID, filesep];
pvp_object_type =  "Car"; %%
pvp_num_ODD_kernels =  3; %%
pvp_edge_type =  "canny"; %%
pvp_bootstrap_str = "_bootstrap"; %% ""; %%
pvp_params_template = ...
    [pvp_input_path, "templates", filesep, ...
     NEOVISION_DATASET_ID, "_", NEOVISION_DISTRIBUTION_ID, "_", ...
     pvp_object_type, num2str(pvp_num_ODD_kernels), pvp_bootstrap_str, "_", pvp_edge_type, "_", ...
     "template.params"];
pvp_list_path = ...
    [pvp_program_path, "list_", pvp_edge_type, filesep];

for i_clip = 1 : length(clip_name)
  disp(clip_name{i_clip});

%% make activity dirs
  pvp_activity_path = ...
      [pvp_program_path, ...
       "activity", filesep];
  mkdir([pvp_activity_path, clip_name{i_clip}, filesep])
  mkdir([pvp_activity_path, clip_name{i_clip}, filesep, ...
	 pvp_object_type, num2str(pvp_num_ODD_kernels), pvp_bootstrap_str, filesep])
  mkdir([pvp_activity_path, clip_name{i_clip}, filesep, ...
	 pvp_object_type, num2str(pvp_num_ODD_kernels), pvp_bootstrap_str, filesep, pvp_edge_type, filesep])

%% make params dirs  
  mkdir([pvp_input_path, clip_name{i_clip}, filesep])
  mkdir([pvp_input_path, clip_name{i_clip}, filesep, ...
	 pvp_object_type, num2str(pvp_num_ODD_kernels), pvp_bootstrap_str, filesep])
  mkdir([pvp_input_path, clip_name{i_clip}, filesep, ...
	 pvp_object_type, num2str(pvp_num_ODD_kernels), pvp_bootstrap_str, filesep, pvp_edge_type, filesep])

  clip_log_dir = [pvp_program_path, ...
		  "log", filesep, clip_name{i_clip}, filesep];
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
    fclose(clip_log_fid);
    pvp_num_frames =  clip_log_struct.tot_clips; %%
    pvp_frame_size =  clip_log_struct.ave_cropped_size; %%
  else
    disp(["~exist clip_log_pathname: ", clip_log_pathname]);
    pvp_num_frames =  450; %%
    pvp_frame_size =  [1080 1920]; %%
  endif %% exist(clip_log_pathname)
  disp(["num_frames = ", num2str(pvp_num_frames)]);
  pvp_fileOfFrames_path = ...
      [pvp_list_path, clip_name{i_clip}, filesep];
  pvp_fileOfFrames_file = ...
      ["train_fileOfFilenames.txt"];
  %%[NEOVISION_DATASET_ID, "_", NEOVISION_DISTRIBUTION_ID, "_", pvp_clip_name, "_", pvp_edge_type, "_", "frames"];
  pvp_fileOfFrames = ...
      [pvp_fileOfFrames_path, pvp_fileOfFrames_file];

  [pvp_params_file, pvp_params_path] = ...
      pvp_makeParams(NEOVISION_DATASET_ID, ...
		     NEOVISION_DISTRIBUTION_ID, ...
		     pvp_repo_path, ...
		     pvp_program_path, ...
		     pvp_input_path, ...
		     clip_name{i_clip}, ...
		     pvp_object_type, ...
		     pvp_num_ODD_kernels, ...
		     pvp_bootstrap_str, ...
		     pvp_edge_type, ...
		     pvp_params_template, ...
		     pvp_frame_size, ...
		     pvp_num_frames, ...
		     pvp_list_path, ...
		     pvp_fileOfFrames);

endfor %% i_clip
