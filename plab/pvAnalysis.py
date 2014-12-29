#########################################
##  pvAnalysis.py
##  Written by Dylan Paiton, Sheng Lundquist
##  Nov 17, 2014
##
## Mimics readpvpfile.m analysis - this scrpt
## will hold a suite of tools for analyzing
## PetaVision output files.
##
#########################################

import scipy.sparse as sparse
import numpy as np
import struct, os, sys
import pdb #TODO: At some point we shouldn't need to include this any more

def read_header_file(fileStream, pos=0):
    fileStream.seek(pos)

    params = struct.unpack("iiiiiiiiiiiiiiiiii", fileStream.read(72))
    header = {}
    header["headersize"] = params[0]
    header["numparams"]  = params[1]
    header["filetype"]   = params[2] # 3 is PVP_WGT_FILE_TYPE
                                     # 4 is PVP_NONSPIKING_ACT_FILE_TYPE
    header["nx"]         = params[3]
    header["ny"]         = params[4]
    header["nf"]         = params[5]
    header["numrecords"] = params[6]
    header["recordsize"] = params[7]
    header["datasize"]   = params[8]
    header["datatype"]   = params[9]  # 3 is float,float,float,...   (dense file)
                                      # 4 is int,float,int,float,... (sparse file)
    header["nxprocs"]    = params[10]
    header["nyprocs"]    = params[11]
    header["nxGlobal"]   = params[12]
    header["nyGlobal"]   = params[13]
    header["kx0"]        = params[14]
    header["ky0"]        = params[15]
    header["nb"]         = params[16]
    header["nbands"]     = params[17]
    header["time"]       = np.fromfile(fileStream,np.float64,1)[0]

    # If hdr.numparams is bigger than 20, there is a field 'additional'
    # containing an vector of hdr.numparams-20 elements of precision int32.
    if header["numparams"] > 20:
        if header["filetype"] == 5: #PVP_KERNEL_FILE has different header setup
            additionalParams = struct.unpack("iiiffi", fileStream.read(24))
            header["nxp"]        = additionalParams[0]
            header["nyp"]        = additionalParams[1]
            header["nfp"]        = additionalParams[2]
            header["wMin"]       = additionalParams[3]
            header["wMax"]       = additionalParams[4]
            header["numPatches"] = additionalParams[5]

            numExtra = (header["headersize"]-80)/4 - 6
            if numExtra > 0: # We still have more information
                header["additional"] = np.fromfile(fileStream,np.int32,numExtra)
            else:
                header["additional"] = []
        else: # all other file types are the same
            header["additional"] = np.fromfile(fileStream,np.int32,header["numparams"]-20)

    return header


def get_frame_info(hdr,fileStream):
    fileSize = os.path.getsize(fileStream.name)
    if hdr["filetype"] == 1:
        frameSize = hdr["recordsize"]*hdr["numrecords"]
        numFrames = (fileSize - hdr["headersize"]) / frameSize
    elif hdr["filetype"] == 2:
        frameSize = -1 # frameSize is a variable
        numFrames = hdr["nbands"]
    elif hdr["filetype"] == 3:
        frameSize = hdr["recordsize"] * hdr["numrecords"] + hdr["headersize"]
        numFrames = fileSize / frameSize
    elif hdr["filetype"] == 4:
        nxprocs   = hdr["nxGlobal"] / hdr["nx"]
        nyprocs   = hdr["nyGlobal"] / hdr["ny"]
        frameSize = hdr["recordsize"] * hdr["datasize"] * nxprocs * nyprocs+8
        numFrames = hdr["nbands"]
    elif hdr["filetype"] == 5:
        frameSize = hdr["recordsize"] * hdr["nbands"] + hdr["headersize"]
        numFrames = fileSize / frameSize
    elif hdr["filetype"] == 6:
        frameSize = -1 # frameSize is a variable
        numFrames = hdr["nbands"]
    return (frameSize,int(numFrames))


def read_dense_data(fileStream, dense_shape, numNeurons):
    #Read timestep
    timestamp = fileStream.read(8)
    if len(timestamp) != 8:
        #EOF
        return (-1, None)
    try:
        idx = struct.unpack("d", timestamp)
        outmat = np.fromfile(fileStream, np.float32, numNeurons).reshape(dense_shape)
        return (idx, outmat)
    except:
        return (-1, None)


def read_sparse_data(fileStream,dense_shape):
    #Function assumes fileStream pointer is in the correct place - so that it can be run in a loop

    timeStamp = fileStream.read(8) # Should be a float64, if not then EOF

    if len(timeStamp) != 8: # EOF
        return (-1,None)

    timeStamp = struct.unpack("d", timeStamp)
    try:
        numActive = np.fromfile(fileStream,np.int32,1)

        if numActive > 0:
            lin_idx   = np.zeros(numActive)
            vals      = np.zeros(numActive)

            #TODO: Speed up by reading in the full string initially
            # File alternates between int32 (index of active cell) and float32 (activity of cell)
            for i in range(numActive):
                lin_idx[i] = np.fromfile(fileStream,np.int32,1)
                vals[i]    = np.fromfile(fileStream,np.float32,1)
                #(idf[i],idx[i],idy[i]) = np.unravel_index(lin_idx[i],shape) # Linear indexing to subscripts

                # Compressed Sparse Column Matrix has efficient column slicing
                ij_mat = (np.zeros(numActive),lin_idx)
                sparseMat = sparse.csc_matrix((vals,ij_mat),shape=(1,dense_shape)) # 1 row, nf*ny*nx columns

        else:
            sparseMat = sparse.csc_matrix(np.zeros(1,dense_shape))
            return (timeStamp, sparseMat)

    except:
        return (timeStamp, None)


def get_pvp_data(fileStream,progressPeriod=0,lastFrame=-1,startFrame=0,skipFrames=1):
    # Usage: (data,hdr) = get_pvp_data(filename,progressperiod,lastFrame,startFrame,skipFrames)
    #
    # ===INPUTS===
    #  filename is a pvp file (any type)
    #
    #  progressPeriod makes a message print every progressPeriod frames.
    #
    #  lastFrame is the index of the last frame to read.  Default is all frames.
    #
    #  startFrame is the starting frame.
    #
    #  skipFrames specifies how many frames should be skipped between each iteration
    #
    # ===OUTPUTS===
    #  outStruct is a structure containing the data and time stamps
    #    outStruct["values"] returns a list containing dense numpy matrices
    #      In general, data has one matrix for each time step written.
    #      For activities, values is an nf-by-nx-by-ny array.
    #      Weights are not supported yet, but they will contain nxp-by-nyp-by-nfp arrays
    #    outStruct["time"] return a list of the timeStamps
    #
    #  hdr is a struct containing the information in the file's header

    hdr = read_header_file(fileStream)

    (frameSize, numFrames) = get_frame_info(hdr,fileStream)

    if lastFrame != -1:
       numFrames = lastFrame

    loopLen   = len(range(startFrame,numFrames,skipFrames))

    data       = loopLen * [None] # This pre-allocates the list
    timeStamps = loopLen * [None]

    if hdr["filetype"] == 3: #PVP_WGT_FILE_TYPE
        #TODO: fill in with real code
        return (None, None)
        pdb.set_trace()

    elif hdr["filetype"] == 4: #PVP_NONSPIKING_ACT_FILE
        if hdr["datatype"] == 3: #PV_FLOAT_TYPE
            shape = (hdr["nf"],hdr["nx"],hdr["ny"])
            numNeurons = shape[0]*shape[1]*shape[2]

            fileStream.seek(startFrame*frameSize)

            fileIdx = 0
            for f in range(startFrame,numFrames):

               if f%skipFrames != 0:
                  fileStream.seek(frameSize)
                  continue

                if progressPeriod != 0:
                    if fileIdx%progressPeriod == 0:
                        sys.stdout.write(" Progress: %d/%d%s"%(fileIdx,loopLen,"\r"))
                        sys.stdout.flush();
                (timeStamps[fileIdx],data[fileIdx]) = read_dense_data(fileStream, shape, numNeurons)
                assert timeStamps[fileIdx] != -1
                assert data[fileIdx]       != None #TODO: FutureWarning: comparison to `None` will result in an elementwise object comparison in the future.
                fileIdx+=1

    elif hdr["filetype"] == 5: #PVP_KERNEL_FILE_TYPE
        #TODO: Program in else case for other datatypes.
        #      Also set up precision variable like in readpvpfile.m
        if hdr["datatype"] == 3: #PV_FLOAT_TYPE (precision is float32)
            precision = 8

            fileStream.seek(startFrame*frameSize,0) # This is in case user doesn't want to start at time=0

            fileIdx = 0
            for f in range(startFrame,numFrames):
                # Need to advance file pointer if frame is to be skipped
                if (f < startFrame) or (f%skipFrames != 0):
                    fileStream.seek(hdr["headersize"]+precision*hdr["nxp"]*hdr["nyp"]*hdr["nfp"]) #seek past the frame
                    continue

                hdr = read_header_file(fileStream,fileStream.tell()) # Header repeats after each frame

                if progressPeriod != 0:
                    if fileIdx%progressPeriod == 0:
                        sys.stdout.write(" Progress: %d/%d%s"%(fileIdx,loopLen,"\r"))
                        sys.stdout.flush();

                tmp_vals_arry = np.zeros((len(range(hdr["nbands"])),len(range(hdr["numPatches"])),hdr["nyp"],hdr["nxp"],hdr["nfp"]))
                for arbor in range(hdr["nbands"]):
                    for patch in range(hdr["numPatches"]):
                        # TODO: Handle shrunken patch info? I have no idea what that entails.
                        #       For now, seek over HyPerLayer's shrunkenPatch data
                        #       uint16 (2 bytes, patch->nx) + uint16 (2 bytes, patch->ny) + uint32 (4 bytes, patch->offset)
                        fileStream.seek(8,1)

                        bytes_to_read = hdr["nxp"]*hdr["nyp"]*hdr["nfp"]
                        tmp_dat       = np.fromfile(fileStream,np.float32,bytes_to_read)
                        # TODO: Figure out if this is flipped from readpvpfile.m
                        tmp_vals_arry[arbor,patch,:,:,:] = np.ravel(tmp_dat).reshape(hdr["nyp"],hdr["nxp"],hdr["nfp"])
                data[fileIdx]       = tmp_vals_arry
                timeStamps[fileIdx] = hdr["time"]
                fileIdx+=1

    elif hdr["filetype"] == 6: #PVP_ACT_SPARSEVALUES_FILE_TYPE
       if hdr["datatype"] == 4: #PV_SPARSEVALUES_TYPE
          frameIdx = 0
          for f in range(numFrames):
               if (f < startFrame) or (f%skipframe != 0):
                  #fseek a frame's worth of bits
                  fileStream.read(8) # Skip over timestamp (Float64)
                  numActive = np.fromfile(fileStream,np.int32,1)
                  fileStream.seek(numActive*8) # Int32 for index, Int32 for value
                  continue

               if progressPeriod != 0:
                   if frameIdx%progressPeriod == 0:
                       sys.stdout.write(" Progress: %d/%d%s"%(frameIdx,loopLen,"\r"))
                       sys.stdout.flush();
               (timeStamps[frameIdx],sparseMat) = read_sparse_data(fileStream,hdr["nf"]*hdr["nx"]*hdr["ny"])

               assert timeStamps[frameIdx] != -1
               assert sparseMat != None

               data[frameIdx] = np.ravel(sparseMat.todense()).reshape(hdr["ny"],hdr["nx"],hdr["nf"])
               frameIdx+=1

    outStruct = {}
    outStruct["time"]   = timeStamps
    outStruct["values"] = data

    sys.stdout.write(" Progress: %d/%d%s"%(numFrames,loopLen,"\r"))
    sys.stdout.write("%s Done.%s"%("\n","\n"))
    sys.stdout.flush();

    return (outStruct,hdr)
