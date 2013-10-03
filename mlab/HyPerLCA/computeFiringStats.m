%%%%%%%%%%%%%%%%%%%%%%
%% computeFiringStats.m
%%   Dylan Paiton
%%   Oct 2, 2013
%%   University of California, Berkeley
%%
%% Inputs:
%%   pvp_filename - exact path/name to the .pvp PetaVision file that you wish to analyze
%%   output_dir   - directory where you would like the results to be saved
%% 
%% Outputs:
%%   Stats
%%
%%%%%%%%%%%%%%%%%%%%%%

%function [statsStruct] = computeFiringStats(pvp_filename, output_dir)

    pvp_filename = '~/Documents/Workspace/HyPerHLCA/output/a3_V1.pvp';
    output_dir = '~/Documents/Workspace/HyPerHLCA/output/mlab';


    %%Variable input
    %if gt(nargin,1) || lt(nargin,1)
    %    error('computeFiringStats: Incorrect number of input arguments.')
    %end

    %%Input PVP file
    if exist(pvp_filename,'file')
       [data,hdr] = readpvpfile(pvp_filename);
    else
        error(['computeFiringStats: ~exist(pvp_filename,"file") in pvp file: ', pvp_filename]);
    end

    numPvFrames = length(data);
    N           = hdr.nx * hdr.ny * hdr.nf;

    if ~exist(output_dir,'dir')
        system(['mkdir -p ',output_dir])
    end

    if exist('data','var') && exist('hdr','var')
        times    = zeros(numPvFrames,1); 
        full_mat = zeros(hdr.ny,hdr.nx,hdr.nf,numPvFrames);
        for y = 1 : 1 : hdr.ny
            for x = 1 : 1 : hdr.nx
                for frame = 1 : 1 : numPvFrames %%Assumes PVP file has contiguous frames
                    times(frame) = squeeze(data{frame}.time);
                    active_ndx   = squeeze(data{frame}.values);
                    vec_mat      = full(sparse(active_ndx+1,1,1,N,1,N)); %%Column vector. PetaVision increments in order: nf, nx, ny
                    rs_mat       = reshape(vec_mat,hdr.nf,hdr.nx,hdr.ny);

                    full_mat(:,:,:,frame) = permute(rs_mat,[3 2 1]); %%Matrix is now [ny, nx, nf]
                end

                [co,lags] = xcorr(squeeze(full_mat(1,1,:,:)));

                correlations.corrs = co;
                correlations.lags = lags;
                save(['analysis_x',num2str(x),'_y',num2str(y)],correlations);
            end
        end
    end
%end
