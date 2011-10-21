
function [num_frames] = pvp_makeCSVFile(CSV_path, ...
					clip_name, ...
					target_name, ...
					distractor_name, ...
					patch_size, ...
					pvp_dir, ...
					plot_trials_skip)
  %% takes PetaVision non-spiking activity files generated in response to
  %% a video clip and produces a CSV file indicating locations of
  %% specified object
  
  object_list = cell(12,1);
  object_list{1} = "Boat"; 
  object_list{2} = "Bus"; 
  object_list{3} = "Car"; 
  object_list{4} = "Container"; 
  object_list{5} = "Cyclist"; 
  object_list{6} = "Helicopter"; 
  object_list{7} = "Person"; 
  object_list{8} = "Plane"; 
  object_list{9} = "Tractor-Trailer"; 
  object_list{10} = "Truck"; 
  object_list{11} = "distractor";  %% non-DARPA object
  object_list{12} = "target"; %% any DARPA object

  machine_path = ...
      [filesep, "Users", filesep, "gkenyon", filesep];

  begin_time = time();

  if nargin < 1 || ~exist("CSV_path") || isempty(CSV_path)
    CSV_path = [macine_path, "Pictures", filesep, "Tower", filesep, ...
		"neovision-chips-tower", filesep]; %% 
  endif
  if nargin < 2 || ~exist("clip_name") || isempty(clip_name)
    clip_name = "Tower_050";
  endif
  if nargin < 3 || ~exist("target_name") || isempty(target_name)
    target_name = "Cyclist";
  endif
  if nargin < 4 || ~exist("distractor_name") || isempty(distractor_name)
    distractor_name = "distractor";
  endif
  if nargin < 5 || ~exist("patch_size") || isempty(patch_size)
    patch_size = [256, 256];
  endif
  if nargin < 6 || ~exist("pvp_dir") || isempty(pvp_dir)
    pvp_dir = [machine_path, "workspace_indigo", filesep, "Clique2", ...
	       filesep, "Tower", filesep, "DoG", filesep, ...
	       target_name, filesep, "test", filesep, clip_name, filesep];
  endif
  if nargin < 7 || ~exist("num_procs") || isempty(num_procs)
    num_procs = 6;  %% 
  endif

  global pvp_dir;
  global patch_size;
 
  setenv('GNUTERM', 'x11');
  image_type = ".png";

  %% get frame IDs 
  clip_dir = [CSV_path, clip_name, filesep];
  if ~exist("clip_dir", "dir")
    error(["~exist(clip_dir):", clip_dir]);
  endif
  frameIDs_path = ...
      [clip_dir, '*', image_type];
  frame_IDs = glob(frameIDs_path);
  num_frames = size(frame_IDs,1);
  disp(['num_frames = ', num2str(num_frames)]);

  if num_procs > 1
    CSV_struct = parcellfun(num_procs, @pvp_makeCSVFileKernel, frame_IDs, "UniformOutput", false);
  endif
  

endfunction %% pvp_makeCSVFile




  

  %% Make the following global parameters available to all functions for convenience.
  global N NROWS NCOLS %% for the current layer
  global NFEATURES  %% for the current layer
  global NO NK dK %% for the current layer
  global ROTATE_FLAG %% x-y axes rotated by DTH / 2 to mitigate aliasing

  global THETA_MAX %% max angle
  THETA_MAX = 1 * pi;

  global COMPRESSED_FLAG
  COMPRESSED_FLAG = 0;

  



  


plot_2AFC_flag = 1;
plot_weights_flag = 1;

global NUM2STR_FORMAT
NUM2STR_FORMAT = '%03.3i';

%% determines whether orientation features are explicitly ploted
global FLAT_ARCH_FLAG
FLAT_ARCH_FLAG = 1;

%% specifies 
global TRAINING_FLAG
TRAINING_FLAG = -1;

%% number of layers of lateral interactions
global G4_FLAG G6_FLAG
G4_FLAG = 1;
G6_FLAG = 0;

global VERBOSE_FLAG
if ~exist("VERBOSE_FLAG") || isempty(VERBOSE_FLAG)
    VERBOSE_FLAG = 0;
  endif
 

kernel_str = 'dirty';

plot_trials_skip = 51;
num_trials = length(frameIDs_file); 

RAW_HIST_FLAG = 1;
RECONSTRUCT_FLAG = 1;
DEBUG_FLAG = 1;

global target_path
target_path = [];
target_path = ...
    [machine_path "Clique2/input/amoebaOnly97x97/test_nosym"];
%%    [machine_path "ODD/input/imageNet/DoG_Mask/test/dog/terrier_vs_antiterrier/trial1"];
%%    [machine_path "kernel/input/256/amoeba/test_target40K_W325_target"];
%%    [machine_path "ODD/input/amoeba/test_target40K_W975_uncompressed_target"];
%%    [machine_path "geisler/input/256/dog/rendered_DoG_test_6/cat_6"]; 
%%    [machine_path "geisler/input/256/cat/likimas512"];    
if ~isempty(target_path)
  target_path = [target_path, G_STR];
  if ((MNIST_flag == 0) &&  (animal_flag == 0) && (dogcat_flag == 0))
    target_path = [target_path, FC_STR, '/'];
  endif
endif % ~isempty(target_path)

if num_trials > plot_trials_skip || RAW_HIST_FLAG
  distractor_path = ...
    [machine_path "Clique2/input/noamoeba97x97/test_nosym"];
%%    [machine_path "ODD/input/imageNet/DoG_Mask/test/cat/terrier_vs_antiterrier/trial1"];
%%    [machine_path, "kernel/input/256/amoeba/test_target40K_W325_distractor"]; 
%%    [machine_path "ODD/input/amoeba/test_target40K_W975_uncompressed_distractor"];
%%    [machine_path "geisler/input/256/cat/rendered_DoG_test_6/dog_6"]; 
%%      []; 
%%      [machine_path, "geisler/input/256/amoeba/distractor40K"]; 
else
  distractor_path = [];
endif
if ~isempty(distractor_path)
  distractor_path = [distractor_path, G_STR];
  if ((MNIST_flag == 0) &&  (animal_flag == 0) && (dogcat_flag == 0))
    distractor_path = [distractor_path, FC_STR, '/'];
  endif
endif % ~isempty(distractor_path)

twoAFC_path = target_path;
SPIKE_PATH = target_path; %[machine_path, 'kernel/input/spiking_target10K', FC_STR];
				%twoAFC_path = [twoAFC_path, G_STR, '/'];
				%spiking_path = [spiking_path, G_STR, '/'];
activity_path = {target_path; distractor_path};
OUTPUT_PATH = twoAFC_path;

min_target_flag = 2 - ~isempty(target_path);
max_target_flag = 1 + ~isempty(distractor_path);

pvp_order = 1;
ROTATE_FLAG = 1;
				% initialize to size of image (if known), these should be overwritten by each layer
NROWS_image=256;
NCOLS_image=256;
NROWS = NROWS_image;
NCOLS = NCOLS_image;
NFEATURES = 8;

NO = NFEATURES; % number of orientations
NK = 1; % number of curvatures
dK = 0; % spacing between curvatures (1/radius)

my_gray = [.666 .666 .666];
num_targets = 1;
fig_list = [];

global NUM_BIN_PARAMS
NUM_BIN_PARAMS = 20;

global NUM_WGT_PARAMS
NUM_WGT_PARAMS = 5;

global N_LAYERS
global SPIKING_FLAG
global pvp_index
SPIKING_FLAG = 0;
[layerID, layerIndex] = pvp_layerID;

read_activity = 2:N_LAYERS;  % list of nonspiking layers whose activity is to be analyzed
num_layers = N_LAYERS;

if RECONSTRUCT_FLAG
  reconstruct_activity = read_activity;
else
  reconstruct_activity = [];
endif

				%acivity_array = cell(num_layers, num_trials);
num_2AFC_tests = 1;
ave_activity = zeros(2, num_layers, num_trials);
std_activity = zeros(2, num_layers, num_trials);
max_activity = zeros(2, num_layers, num_trials);
mnz_activity = zeros(2, num_layers, num_trials);
snz_activity = zeros(2, num_layers, num_trials); % sum of positive
				% values only
global num_hist_bins
num_hist_bins = 100;
hist_activity_bins = cell(num_layers, 1);
for layer = 1 : num_layers
  hist_activity_bins{layer} = []; 
endfor
hist_activity = zeros(2, num_hist_bins, num_layers);
hist_activity_cell = cell(2, num_layers);
act_time = zeros(num_layers, num_trials);
				% 1 == ave, 2 == max, 3 ==
				% ave > 0, 4 == sum > 0
twoAFC_test_str = cell(4, 1);
twoAFC_test_str{1} = 'ave  ';
twoAFC_test_str{2} = 'sum  ';
twoAFC_test_str{3} = 'mnz';
twoAFC_test_str{4} = 'snz';

twoAFC = zeros(2, num_layers, num_trials, num_2AFC_tests);

num_rows = ones(num_layers, num_trials);
num_cols = ones(num_layers, num_trials);
num_features = ones(num_layers, num_trials);
pvp_layer_header = cell(N_LAYERS, 1);

activity = cell(2,1);

raw_hist_count = 0;
reconstruct_count = 0;

for j_trial = first_trial : skip_trial : last_trial    
  
  %% Analyze activity layer by layer
  for layer = read_activity;
    
    %% layer names, L -> retina, V1; G->lateral; T->topdown
    if layer <= 3
      layer_level = layer - 1;
    else
      layer_level = layer - 3 - ( 3 + G4_FLAG ) * ( layer > (6 + G4_FLAG) );
    endif % layer < 3
    if layer <= 3
      layer_label = 'L';
    elseif layer <= ( 6 + G4_FLAG + 2 * G6_FLAG)
      layer_label = 'ODD_';
    else
      layer_label = 'T';
    endif % layer < 3

    %% account for delays between layers
    if TOPDOWN_FLAG
      i_trial = j_trial + (layer - 1) * (layer <= ( 6 + G4_FLAG ) ) + ...
	  (layer - 1 - 3 - G4_FLAG + 2) * (layer > ( 6 + G4_FLAG ) );
    else
      i_trial = j_trial + (layer - 1);
    endif %TOPDOWN_FLAG
    
    for target_flag = min_target_flag : max_target_flag
      
      OUTPUT_PATH = activity_path{target_flag};
      activity{target_flag} = [];
      
      %% Read spike events
      hist_bins_tmp = ...
          hist_activity_bins{layer};
      %%keyboard;
      [act_time(layer, j_trial),...
       activity{target_flag}, ...
       hist_activity_tmp, ...
       hist_activity_bins{layer}, ...
       pvp_layer_header{layer, 1}] = ...
          pvp_readActivity(layer, i_trial, hist_bins_tmp, pvp_order);

      disp([ layerID{layer}, ...
	    ': ave_activity(', num2str(layer), ',', num2str(j_trial), ') = ', ...
	    num2str(ave_activity(target_flag, layer, j_trial) ) ]);

      hist_activity(target_flag, :, layer) = ...
          hist_activity(target_flag, :, layer) + ...
          hist_activity_tmp;
      
      hist_activity_cell{target_flag, layer} = hist_activity_tmp;
      
      if isempty(activity{target_flag}) || ~any(activity{target_flag})
        continue;
      endif

      ave_activity(target_flag, layer, j_trial) = ...
	  mean( activity{target_flag}(:) );
      std_activity(target_flag, layer, j_trial) = ...
	  std( activity{target_flag}(:) );
      sum_activity(target_flag, layer, j_trial) = ...
	  sum( activity{target_flag}(:) );
      mnz_activity(target_flag, layer, j_trial) = ...
	  mean( activity{target_flag}( activity{target_flag} > 0 ) );
      snz_activity(target_flag, layer, j_trial) = ...
	  mean( activity{target_flag}( activity{target_flag} > 0.5 ) );

      if DEBUG_FLAG
	subindex_str = ['(', num2str(target_flag), ',', ...
			  num2str(layer), ',', ...
			  num2str(j_trial), ') = '];			  
	disp([layerID{layer}, ': ', ...
	      'ave_activity', ...
	      subindex_str, ...
	      num2str(ave_activity(target_flag, layer, j_trial))]);
	disp([layerID{layer}, ': ', ...
	      'std_activity', ...
	      subindex_str, ...
	      num2str(std_activity(target_flag, layer, j_trial))]);
	disp([layerID{layer}, ': ', ...
	      'sum_activity', ...
	      subindex_str, ...
	      num2str(sum_activity(target_flag, layer, j_trial))]);
	disp([layerID{layer}, ': ', ...
	      'mnz_activity', ...
	      subindex_str, ...
	      num2str(mnz_activity(target_flag, layer, j_trial))]);
	disp([layerID{layer}, ': ', ...
	      'snz_activity', ...
	      subindex_str, ...
	      num2str(snz_activity(target_flag, layer, j_trial))]);
      endif

      write_activity_flag = 0;
      zip_activity_flag = 1;
      if write_activity_flag == 1
        if (zip_activity_flag == 1)
          activity_filename = ['V1_G', num2str(layer-1),'_', ...
			       num2str(j_trial, NUM2STR_FORMAT), '.mat.z']
          activity_filename = [OUTPUT_PATH, activity_filename]
          %%save("-z", "-mat", activity_filename, "activity" );
          save('-mat', activity_filename, 'activity{target_flag}' );
        else
          activity_filename = ['V1_G', num2str(layer-1), '_', ...
			       num2str(j_trial, NUM2STR_FORMAT), '.mat']
          activity_filename = [OUTPUT_PATH, activity_filename]
          save('-mat', activity_filename, 'activity{target_flag}' );
        endif
        
      endif
      
      
      num_rows(layer, j_trial) = NROWS;
      num_cols(layer, j_trial) = NCOLS;
      num_features(layer, j_trial) = NFEATURES;
      
				% plot reconstructed image
      reconstruct_activity2 = ...
	  ismember( layer, reconstruct_activity ) && ...
	  ( mod(reconstruct_count-1,plot_trials_skip) == 0 ); % * ...
%%	  ( reconstruct_count <= plot_trials_skip ); % * ...
      if reconstruct_activity2
        size_activity = ...
	    [ 1 , num_features(layer, j_trial), ...
	     num_cols(layer, j_trial), num_rows(layer, j_trial) ];
        recon_filename = ...
	    ['recon ', layer_label, num2str(layer_level), '_', ...
	     num2str(j_trial, NUM2STR_FORMAT), '_', ...
	     num2str(target_flag)];
	fig_tmp = figure;
	set(fig_tmp, 'Name', recon_filename);
        fig_tmp = ...
	    pvp_reconstruct(activity{target_flag}, ...
			    recon_filename, fig_tmp, ...
			    size_activity, ...
			    1);
        fig_list = [fig_list; fig_tmp];
	
      endif
      
      twoAFC(target_flag, layer, j_trial, 1) = ...
	  ave_activity(target_flag, layer, j_trial);
      twoAFC(target_flag, layer, j_trial, 2) = ...
	  sum_activity(target_flag, layer, j_trial);
      twoAFC(target_flag, layer, j_trial, 3) = ...
	  mnz_activity(target_flag, layer, j_trial);
      twoAFC(target_flag, layer, j_trial, 4) = ...
	  snz_activity(target_flag, layer, j_trial);

    endfor  % target_flag
    
  endfor  % layer
  
  if RAW_HIST_FLAG && ( mod(raw_hist_count-1,plot_trials_skip)==0 )
    
    raw_hist_filename = ...
        ['raw hist ', ...
         num2str(j_trial, NUM2STR_FORMAT)];
    fig_tmp = figure('Name', raw_hist_filename);
    fig_list = [fig_list; fig_tmp];
    
    for layer = read_activity;
      
      for target_flag = min_target_flag : max_target_flag

	subplot_index = find(read_activity == layer);	
	subplot(length(read_activity), 1, subplot_index);
	hold on
	if target_flag == 1
	  red_hist = 1;
	  blue_hist = 0;
	  bar_width = 0.8;
	else
	  red_hist = 0;
	  blue_hist = 1;
	  bar_width = 0.6;
        endif
	if any(hist_activity_cell{target_flag,layer}(:))
	  bh = bar(hist_activity_bins{layer, 1}, ...
		   log( squeeze( hist_activity_cell{target_flag,layer}(:) ) + 1), ...
		   bar_width);
	  hold on
	  set( bh, 'EdgeColor', [red_hist 0 blue_hist] );
	  set( bh, 'FaceColor', [red_hist 0 blue_hist] );
	endif
	
      endfor  % target_flag
    endfor  % layer
  endif % raw_hist_flag
  
  reconstruct_count = reconstruct_count + 1;
  raw_hist_count = raw_hist_count + 1;
   
  pvp_saveFigList( fig_list, OUTPUT_PATH, 'png');
  close all;
  fig_list = [];

endfor % j_trial


twoAFC_filename = ...
    ['twoAFC', num2str(expNum), '.mat']
twoAFC_filename = [twoAFC_path, twoAFC_filename]
if plot_2AFC_flag == 1
  if exist("twoAFC") && ~isempty(twoAFC)
    save('-mat', twoAFC_filename, 'twoAFC', 'tot_trials', ...
	 'ave_activity', 'std_activity', 'sum_activity', 'mnz_activity', 'snz_activity');
  elseif exist(twoAFC_filename,"file")
    load(twoAFC_filename);
  else
    disp("twoAFC not found");
    plot_2AFC_flag = 0;
  endif
endif

%% 2AFC analysis

max_target_flag = size(twoAFC, 1);
min_target_flag = 1;
tot_trials = size(twoAFC, 3); %length( first_trial : skip_trial : num_trials );
plot_hist_activity_flag = 0 && plot_2AFC_flag;
%%plot_2AFC_flag = (tot_trials > plot_trials_skip) && plot_2AFC_flag;
if max_target_flag > min_target_flag

  if plot_hist_activity_flag
    
    subplot_index = 0;
    num_subplots = length(read_activity);
    hist_name = 'Cum Pixel Dist';
    fig_tmp = figure('Name', hist_name);
    fig_list = [fig_list; fig_tmp];
    for layer = read_activity
      subplot_index = subplot_index + 1;
      subplot(num_subplots, 1, subplot_index);
      hist_activity_tmp = ...
	  hist_activity(1, :, layer) / ...
	  sum( squeeze( hist_activity(1, :, layer) ) );
      cum_activity_target = ...
	  1 - cumsum( hist_activity_tmp );
      hist_activity_tmp = ...
	  hist_activity(2, :, layer) / ...
	  sum( squeeze( hist_activity(2, :, layer) ) );
      cum_activity_distractor = ...
	  1 - cumsum( hist_activity_tmp );
      twoAFC_correct = 0.5 + 0.5 * ...
	  ( cum_activity_target - cum_activity_distractor );
      bar( hist_activity_bins{layer}, twoAFC_correct );  
    endfor
    hist_filename = ...
	['hist_activity', '.mat']
    hist_filename = [OUTPUT_PATH, hist_filename]
    %%save("-z", "-mat", hist_filename, "hist_activity" );
    save('-mat', hist_filename, 'hist_activity');

  endif

  
  if plot_2AFC_flag

    artificial_trials = [1:75, 151:225, 301:375, 451:525];
    natural_trials = [76:150, 226:300, 376:450, 526:599];
    body_trials = [1:150];
    far_trials = [151:300];
    head_trials = [301:450];
    middle_trials = [451:599];
    all_trials =  [1:size(twoAFC,3)];
    twoAFC_trials = all_trials; %%all_trials; 
        
    mean_2AFC = squeeze( mean( twoAFC(:,:,twoAFC_trials,:), 3 ) );
    std_2AFC = squeeze( std( twoAFC(:,:,twoAFC_trials,:), 0, 3 ) );

    twoAFC_correct = zeros(num_layers, size(twoAFC,4));
    twoAFC_errorbar = zeros(num_layers, size(twoAFC,4));

    baseline_layer = 3; %% 0;
    percent_change_flag = 1; %%0;
    cum_change_flag = 1; %%0;

    twoAFC_list = [1];
    for i_2AFC_test = twoAFC_list %%1 : num_2AFC_tests
      
      [twoAFC_hist, twoAFC_bins, twoAFC_calc] = ...
	  pvp_calc2AFCHist(twoAFC(:,:,twoAFC_trials,i_2AFC_test), ...
			   read_activity, ...
			   1, ...
			   twoAFC_test_str{i_2AFC_test}, ...
			   baseline_layer, ...
			   percent_change_flag, ...
			   cum_change_flag);
      [fig_list_tmp] = ...
	  pvp_plot2AFCHist(twoAFC_hist, ...
			   twoAFC_bins, ...
			   read_activity, ...
			   twoAFC_test_str{i_2AFC_test});
      fig_list = [fig_list; fig_list_tmp];
      
      [twoAFC_cumsum, twoAFC_ideal] = ...
	  pvp_calc2AFCCumsum(twoAFC_hist, ...
			     read_activity, ...
			     1, ...
			     twoAFC_test_str{i_2AFC_test});
      plot_ideal = 0;
      if plot_ideal
      [fig_list_tmp] = ...
	  pvp_plot2AFCIdeal(twoAFC_ideal, ...
			    twoAFC_bins, ...
			    read_activity, ...
			    1, ...
			    twoAFC_test_str{i_2AFC_test});
      fig_list = [fig_list; fig_list_tmp];
      endif

      
      [twoAFC_ROC, twoAFC_AUC] = ...
	  pvp_calc2AFCROC(twoAFC_cumsum, ...
			  read_activity, ...
			  1, ...
			  twoAFC_test_str{i_2AFC_test});
      [fig_list_tmp] = ...
	  pvp_plot2AFCROC(twoAFC_ROC, ...
			  read_activity, ...
			  1, ...
			  twoAFC_test_str{i_2AFC_test});
      fig_list = [fig_list; fig_list_tmp];

      
      disp(twoAFC_test_str{i_2AFC_test});
      for layer = read_activity
	for target_flag = 1 : 2;
	  disp( ['mean_2AFC(', num2str(target_flag), ',', ...
			    num2str(layer), ',', num2str(i_2AFC_test), ') = ', ...
		 num2str(mean_2AFC(target_flag, layer, i_2AFC_test)), ...
		 '+/- ', 
		 num2str(std_2AFC(target_flag, layer, i_2AFC_test)), ...
		 ] );
	endfor
      endfor
      for layer = read_activity
	twoAFC_tmp = ...
	    squeeze(twoAFC_calc(:, layer, :, i_2AFC_test));
	twoAFC_correct(layer, i_2AFC_test) = ...
	    sum( squeeze(twoAFC_tmp(1, twoAFC_trials) >
			 twoAFC_tmp(2, twoAFC_trials) ) ) / ...
	    ( length(twoAFC_trials) + (length(twoAFC_trials) == 0) );
	if ( twoAFC_correct(layer, i_2AFC_test) * length(twoAFC_trials) ) ~= 0
	  twoAFC_errorbar(layer, i_2AFC_test) = ...
	      sqrt( 1 - twoAFC_correct(layer, i_2AFC_test) ) / ...
	      sqrt(twoAFC_correct(layer, i_2AFC_test) * ...
		   length(twoAFC_trials) );
	else
	  twoAFC_errorbar(layer, i_2AFC_test) = 0;
	endif
	disp( ['twoAFC_correct(', num2str(layer), ...
			       ',', num2str(i_2AFC_test), ') = ', ...
	       num2str(twoAFC_correct(layer, i_2AFC_test)), ...
	       "+/-", ...
	       num2str(twoAFC_errorbar(layer, i_2AFC_test))] );
	disp( ['twoAFC_AUC(', num2str(layer), ...
			       ',', num2str(i_2AFC_test), ') = ', ...
	       num2str(twoAFC_AUC(layer, 1))] );
      endfor %% layer
      
    endfor % i_2AFC_test
    
    save('-mat', twoAFC_filename, 'twoAFC', 'twoAFC_hist', ...
	 'twoAFC_cumsum', 'twoAFC_ideal', 'twoAFC_correct',
	 'twoAFC_errorbar', ...
	 'twoAFC_ROC', ...
	 'twoAFC_AUC', ...
	 'tot_trials', ...
	 'ave_activity', 'std_activity', 'sum_activity', 'mnz_activity', 'snz_activity');
    
  endif

  pvp_saveFigList( fig_list, twoAFC_path, 'png');
  fig_list = [];
  
endif




%% plot connections
if plot_weights_flag == 1
  global N_CONNECTIONS
  global NXP NYP NFP
  global NUM_ARBORS
  NUM_ARBORS = 2;
  [connID, connIndex, num_arbors] = pvp_connectionID();
  if TRAINING_FLAG > 0
    plot_weights = []; %N_CONNECTIONS;
  else
    if max_target_flag > min_target_flag
      plot_weights = ( N_CONNECTIONS ) : ( N_CONNECTIONS+(TRAINING_FLAG<=0) );
    else
      plot_weights = ( N_CONNECTIONS ) : ( N_CONNECTIONS+(TRAINING_FLAG<=0) );
    endif
  endif
  weights = cell(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  weight_invert = ones(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  weight_invert(5,2) = -1; %% 
  weight_invert(7,2) = -1; %%-1;
  weight_invert(9,2) = -1; %%-1;
  pvp_conn_header = cell(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  nxp = cell(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  nyp = cell(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  FLAT_ARCH_FLAG = 0;
  write_pvp_kernel_flag = 1;
  write_mat_kernel_flag = 1;
  weight_type = cell(N_CONNECTIONS+(TRAINING_FLAG<=0), NUM_ARBORS);
  for i_conn = plot_weights
    for i_arbor = 1 : num_arbors(i_conn)
      if num_arbors(i_conn) == 1;
	j_arbor = 0;
      else
	j_arbor = i_arbor;
      endif
      weight_type{i_conn, i_arbor} = kernel_str;
      weight_min = 10000000.;
      weight_max = -10000000.;
      weight_ave = 0;
      if i_conn < N_CONNECTIONS+1
	[weights{i_conn, i_arbor}, nxp{i_conn, i_arbor}, nyp{i_conn, i_arbor}, ...
	 pvp_conn_header{i_conn, i_arbor}, pvp_index ] ...
	    = pvp_readWeights(i_conn, j_arbor);
	pvp_conn_header_tmp = pvp_conn_header{i_conn, i_arbor};
	NXP = pvp_conn_header_tmp(pvp_index.WGT_NXP);
	NYP = pvp_conn_header_tmp(pvp_index.WGT_NYP);
	NFP = pvp_conn_header_tmp(pvp_index.WGT_NFP);
	if NXP <= 1 || NYP <= 1
	  continue;
	endif
	num_patches = pvp_conn_header_tmp(pvp_index.WGT_NUMPATCHES);
	for i_patch = 1:num_patches
	  weight_min = min( min(weights{i_conn,i_arbor}{i_patch}(:)), weight_min );
	  weight_max = max( max(weights{i_conn,i_arbor}{i_patch}(:)), weight_max );
	  weight_ave = weight_ave + mean(weights{i_conn, i_arbor}{i_patch}(:));
	endfor
	weight_ave = weight_ave / num_patches;
	disp( ['weight_min = ', num2str(weight_min)] );
	disp( ['weight_max = ', num2str(weight_max)] );
	disp( ['weight_ave = ', num2str(weight_ave)] );
	if ~TRAINING_FLAG
	  continue;
	endif
      elseif i_conn == N_CONNECTIONS + 1
	w_max_target = max(weight_invert(i_conn-1,1)*weights{i_conn-1,1}{i_patch}(:));
	w_max_distractor = max(weight_invert(i_conn-1,2)*weights{i_conn-1,2}{i_patch}(:));
	disp('calculating geisler kernels');
	pvp_conn_header{i_conn} = pvp_conn_header{i_conn-1,1};
	pvp_conn_header_tmp = pvp_conn_header{i_conn,1};
	num_patches = pvp_conn_header_tmp(pvp_index.WGT_NUMPATCHES);
	nxp{i_conn} = nxp{i_conn-1,1};
	nyp{i_conn} = nyp{i_conn-1,1};
	for i_patch = 1:num_patches
	  weights{i_conn,i_arbor}{i_patch} = ...
	      (weight_invert(i_conn-1,1)*weights{i_conn-1,1}{i_patch} - ...
	       weight_invert(i_conn-1,2)*weights{i_conn-1,2}{i_patch});
	  weight_min = min( min(weights{i_conn,1}{i_patch}(:)), weight_min );
	  weight_max = max( max(weights{i_conn,1}{i_patch}(:)), weight_max );
	  weight_ave = weight_ave + mean(weights{i_conn,1}{i_patch}(:));
	endfor
	weight_ave = weight_ave / num_patches;
	disp( ['weight_min = ', num2str(weight_min)] );
	disp( ['weight_max = ', num2str(weight_max)] );
	disp( ['weight_ave = ', num2str(weight_ave)] );
	if write_pvp_kernel_flag
	  %%keyboard;
	  NCOLS = pvp_conn_header_tmp(pvp_index.NX);
	  NROWS = pvp_conn_header_tmp(pvp_index.NY);
	  NFEATURES = pvp_conn_header_tmp(pvp_index.NF);
	  N = NROWS * NCOLS * NFEATURES;
	  weights_size = [ NFP, NXP, NYP];
	  pvp_writeKernel( weights{i_conn,i_arbor}, weights_size, kernel_str );
	endif % write_pvp_kernel_flag
      else
	continue;
      endif  % i_conn < N_CONNECTIONS + 1
      if write_mat_kernel_flag
	mat_weights = weights{i_conn,i_arbor};
	mat_weights_filename = ...
	    [kernel_str, num2str(expNum), '_', num2str(i_conn), '_a', num2str(i_arbor), '.mat']
	mat_weights_filename = [twoAFC_path, mat_weights_filename]
	%%save("-z", "-mat", geisler_weights_filename, "geisler_weights");
	save('-mat', mat_weights_filename, 'mat_weights');
      endif % write_mat_kernel_flag
      NK = 1;
      NO = floor( NFEATURES / NK );
      skip_patches = 1; %%num_patches;
      for i_patch = 1 : skip_patches : num_patches
	NCOLS = nxp{i_conn, i_arbor}(i_patch);
	NROWS = nyp{i_conn, i_arbor}(i_patch);
	N = NROWS * NCOLS * NFEATURES;
	patch_size = [1 NFEATURES  NCOLS NROWS];
	NO = NFP;
	plot_name = ...
	    [connID{i_conn}, '(', ...
			       int2str(i_conn), ',', ...
			       int2str(i_arbor), ',', ...
			       int2str(i_patch), ')' ];
	pixels_per_cell = 5;
	[fig_tmp, recon_colormap] = ...
	    pvp_reconstruct(weights{i_conn,i_arbor}{i_patch}*weight_invert(i_conn,i_arbor), ...
			    plot_name, ...
			    [], patch_size, [], ...
			    pixels_per_cell);
	fig_list = [fig_list; fig_tmp];
	plot_recon_subsection = 0;
	plot_recon_subsection
	if plot_recon_subsection
	  weights_sub = ...
	      weights{i_conn,i_arbor}{i_patch}*weight_invert(i_conn,i_arbor);
	  weights_sub = ...
	      reshape(weights_sub, patch_size);
	  sub_size = floor( NROWS / 8 );
	  row_start = 1 + sub_size;
	  row_end = row_start + sub_size - 1;
	  col_start = 1 + sub_size;
	  col_end = col_start + sub_size - 1;
	  weights_sub = ...
	      weights_sub(1,:, col_start:col_end, row_start:row_end);
	  patch_size_sub = [1 NFEATURES  sub_size sub_size];
	  plot_name_sub = ...
	      [connID{i_conn}, '(', ...
				 int2str(i_conn), ',', ...
				 int2str(i_arbor), ',', ...
				 int2str(i_patch), ')', ...
	       " insert"];
	  pixels_per_cell_sub = 5*sub_size;
	  [fig_sub, recon_colormap] = ...
	      pvp_reconstruct(weights_sub(:), ...
			      plot_name_sub, ...
			      [], patch_size_sub, [], ...
			      pixels_per_cell_sub);
	  fig_list = [fig_list; fig_sub];      
	endif
      endfor %% i_patch
    endfor %% i_arbor
  endfor %% i_conn
  FLAT_ARCHITECTURE = 0;

  pvp_saveFigList( fig_list, OUTPUT_PATH, 'png');
  close all;
  fig_list = [];
endif