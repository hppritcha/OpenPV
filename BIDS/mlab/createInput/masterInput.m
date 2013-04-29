clear all;
close all;
addpath('./k-Wave Toolbox');

SIMULATION_FILENAME = './simulation_output.mat';
NOISE_FILENAME       = './noise_output.mat';

DIM  = [512, 512, 0];   % [X, Y, t=0] This will be a 320x3200m grid for the simulation
dx   = 0.625;            % [m/px]
dy   = dx;              % [m/px]
BETA = -1;              % 0 is gaussian white, -1 is pink, -2 is Brownian

%Sine wave params
% peak vehicle frequency
WAVE_FREQUENCY = 125;   %Hz
WAVE_STRENGTH  = 0.11;  %Pa (75dB SBL)

%Time properties
SOURCE_VEL  = 8.9408;   % [m/s] = 20 mph
TIME_LENGTH = 30;       % [s]
dt          = 10e-3;    % [s] - 10ms

%Medium properties
% pure tone through air at 20 deg C, 30 perc humidity, 4000ft elevation (0.8755 bars, 0.864 ATM)
medium.sound_speed = 348.9; % [m/s]
medium.alpha_coeff = 404;   % [dB/MHz^y cm]
medium.alpha_power = 1.9;     % y

%Source mask params
DROP_RADIUS = 1; %~2.5m radius
DROP_POS    = [1, DIM(2)/2+1]; %[X, Y] - NOTE: 1 indexed

MOVIE_NAME  = '~/plot';
OUTPUT_DIR  = '~/wave_stimulus';
NOISE_SCALE = .4; % (1 - NOISE_SCALE) = SNR (i.e. 80% is 0.2)

CLOBBER_SIMULATION = 0;
CLOBBER_NOISE      = 1;

if ne(exist(OUTPUT_DIR),7)
   mkdir(OUTPUT_DIR);
end

if ~exist(SIMULATION_FILENAME) || CLOBBER_SIMULATION
    disp('masterInput: Creating wave input...')

    %Create input wave
    createInput;

    %Remove orig drop from matrix
    orig_drop = 1 - orig_drop;
    all_wave = bsxfun(@times, orig_drop, all_wave); %%TODO: This might be cheating - check with Gar
    disp('Saving simulation output...')
    save(SIMULATION_FILENAME,'all_wave','-v7.3');
else
    disp('masterInput: Loading wave input...')
    load(SIMULATION_FILENAME);
end

[Y, X, Z] = size(all_wave);
DIM(3) = Z*2; %Need an equal set of noisy frames without the stimulus
disp('masterInput: Scaling simulation...')

%Scale wave
range_wave = max(all_wave(:)) - min(all_wave(:));
new_wave   = all_wave ./ range_wave;
long_wave  = zeros(DIM);
long_wave(:,:,DIM(3)/2+1:end) = new_wave;

clearvars -except long_wave DIM BETA OUTPUT_DIR NOISE_SCALE NOISE_FILENAME CLOBBER_NOISE

if ~exist(NOISE_FILENAME) || CLOBBER_NOISE
    %Generate noise
    disp('masterInput: Creating noise...');
    all_noise = spatialPattern(DIM, BETA);

    %Scale noise
    std_noise = std(all_noise(:));
    new_noise = all_noise ./ std_noise;
    new_noise = new_noise .* NOISE_SCALE;
    disp('masterInput: Saving noise output...')
    save(NOISE_FILENAME,'new_noise','-v7.3');
else
    disp('masterInput: Loading noise...');
    load(NOISE_FILENAME);
end

disp('masterInput: Combining noise with simulation output...')
%Scale input for imwrite
new_input    = long_wave + new_noise;
clearvars -except new_input DIM OUTPUT_DIR

scale        = (max([abs(max(new_input(:))) abs(min(new_input(:)))]) * 2);
scaled_input = new_input ./ scale;
scaled_input = scaled_input .* 255; 
scaled_input = scaled_input + 128;
scaled_input = uint8(floor(scaled_input));

disp('masterInput: Writing files...');
%h = waitbar(0, 'Writing files...');
for i = 1:DIM(3)
    frame_str = sprintf('%05d',i);
    imwrite(abs(scaled_input(:,:,i)),[OUTPUT_DIR,'/input_',frame_str,'.jpg']);
    %waitbar(i/DIM(3));
end
%close(h);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clearvars -except new_input
disp('masterInput: Done.');
