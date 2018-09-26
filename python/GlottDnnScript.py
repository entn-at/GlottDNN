#!/usr/bin/python

from __future__ import division

import sys
import os
import numpy as np
import random
import wave
import math
import imp # for importing argv[1]
import warnings

# Config file 
if len(sys.argv) < 2:
    sys.exit("Usage: python GlottDnnScript.py config.py")
if os.path.isfile(sys.argv[1]):
    conf = imp.load_source('', sys.argv[1])
else:
    sys.exit("Config file " + sys.argv[1] + " does not exist")

# Import Theano-based DNN training if used 
if conf.do_dnn_training:
    import TrainDnn
    
# Import dynamic numpy data provider and Keras training    
try:
    if conf.make_data_provider:
            import data_utils as dutil
            import TrainDnnKeras as ktrain  
except AttributeError:
        warnings.warn("Config file does not have attribute 'make_data_provider'" )    

def write_dnn_infofile():
    f = open(conf.weights_data_dir + '/' + conf.dnn_name + '.dnnInfo', 'w')
    # write layer sizes
    n_input = sum(conf.input_dims)
    n_output = sum(conf.output_dims)
    f.write('LAYERS = [')
    f.write(str(n_input) + ', ')
    for n_hidden in conf.n_hidden:
        f.write(str(n_hidden) + ', ')
    f.write(str(n_output))
    f.write('];\n')
    # write activations
    n_input = sum(conf.input_dims)
    n_output = sum(conf.output_dims)
    f.write('ACTIVATIONS = [')
    #f.write(str(n_input) + ', ')
    for n_hidden in conf.n_hidden:
        f.write( '\"S\", ')
    f.write('\"L\", \"L\"')
    f.write('];\n')    
    # write feature orders 
    f.write('F0_ORDER = ' + str(conf.input_dims[0]) + ';\n')
    f.write('GAIN_ORDER = ' + str(conf.input_dims[1]) + ';\n')
    f.write('HNR_ORDER = ' + str(conf.input_dims[2]) + ';\n')
    f.write('LPC_ORDER_GLOT = ' + str(conf.input_dims[3]) + ';\n')
    f.write('LPC_ORDER_VT = ' + str(conf.input_dims[4]) + ';\n')
    f.write('SAMPLING_FREQUENCY = ' + str(conf.sampling_frequency) + ';\n')
    f.write('WARPING_LAMBDA_VT = ' + str(conf.warping_lambda) + ';\n')
    f.close()

def mkdir_p(dirpath):
    if not os.path.exists(dirpath):
        os.makedirs(dirpath)

def make_directories():
    # Prepare environment 
    """
    mkdir_p(conf.datadir + '/wav')
    mkdir_p(conf.datadir + '/raw')
    mkdir_p(conf.datadir + '/gci')
    mkdir_p(conf.datadir + '/scp')
    mkdir_p(conf.datadir + '/exc')
    mkdir_p(conf.datadir + '/syn')
    mkdir_p(conf.datadir + '/f0')
    mkdir_p(conf.datadir + '/gain')
    mkdir_p(conf.datadir + '/lsf')
    mkdir_p(conf.datadir + '/slsf')
    mkdir_p(conf.datadir + '/hnr')
    mkdir_p(conf.datadir + '/pls')
    """

    dirs = ['wav',
            'raw',
            'gci',
            'scp',
            'exc',
            'syn',
            'f0',
            'gain',
            'lsf',
            'slsf',
            'hnr',
            'pls']

    for d in dirs:
        mkdir_p(os.path.join(conf.datadir, d))            

    for t in conf.inputs:
        mkdir_p(conf.datadir + '/' + t)
    for t in conf.outputs:
        mkdir_p(conf.datadir + '/' + t)
        
    # Dnn directiories
    mkdir_p(conf.train_data_dir)
    mkdir_p(conf.weights_data_dir)
            
def make_file_lists():
    
    scp_types = ['wav']
    scp_types.extend(conf.inputs)
    scp_types.extend(conf.outputs)
    
    extensions = ['.wav']
    extensions.extend(conf.input_exts)
    extensions.extend(conf.output_exts)
    
    for t,e in zip(scp_types, extensions):
        scpfile = open(conf.datadir + '/scp/' + t + '.scp','w') 
        for f in sorted(set(os.listdir(conf.datadir + '/' + t))):
            if f.endswith(e):
                scpfile.write(os.path.abspath(conf.datadir + '/' + t + '/' + f + '\n'))
        scpfile.close()
        
    """
    scpfile = open(conf.datadir + '/scp/wav.scp','w') 
    for f in sorted(set(os.listdir(conf.datadir + '/wav'))):
        if f.endswith(".wav"):
            scpfile.write(os.path.abspath(conf.datadir + '/wav/' + f + '\n'))
    scpfile.close()
    """

def sptk_pitch_analysis():
        wavscp = conf.datadir + '/scp/wav.scp'
        with open(wavscp,'r') as wavfiles:
            for f in wavfiles:
                wavfile = f.rstrip()
                if os.path.isfile(wavfile):
                    bname = os.path.splitext(os.path.basename(wavfile))[0]
                    print("SPKT pitch estimation for {}".format(bname))
                    # convert to .raw
                    rawfile = conf.datadir + '/raw/' + bname + '.raw'
                    f0file = conf.datadir + '/f0/' + bname + '.f0'
                    cmd = conf.sox + ' ' + wavfile + ' ' + rawfile
                    os.system(cmd)
                    # sptk pitch estimation (first pass)
                    cmd = conf.x2x + ' +sf ' + rawfile + '|' \
                        + conf.pitch + ' -L 50 -H 500 ' + '>' + f0file
                    os.system(cmd)
                    # pitch range estimation for second pass
                    f0 = np.fromfile(f0file, dtype=np.float32)
                    lf0 = np.log10(f0[f0>0])
                    m = lf0.mean()
                    s = lf0.std()
                    f0_max = np.round(10.0**(m+3.0*s))
                    #f0_min = np.round(10.0**(m-3.0*s))
                    f0_min = 50.0 # minimum f0 is low for creaky voice
                    # sptk pitch estimation (second pass)
                    print("   second pass with F0 range {}-{} Hz".format(f0_min, f0_max)) 
                    cmd = conf.x2x + ' +sf ' + rawfile + '|' \
                        + conf.pitch + ' -L ' + str(f0_min) + ' -H ' + str(f0_max) \
                        + ' > ' + f0file
                    os.system(cmd)
                    
    
def reaper_pitch_analysis():
    wavscp = conf.datadir + '/scp/wav.scp'
    with open(wavscp,'r') as wavfiles:
        for f in wavfiles:
            wavfile = f.rstrip()
            if os.path.isfile(wavfile):
                # define paths
                bname = os.path.splitext(os.path.basename(wavfile))[0]
                f0tmp1 = conf.datadir + '/f0/' + bname + '.f0tmp1'
                f0tmp2 = conf.datadir + '/f0/' + bname + '.f0tmp2'                
                f0file = conf.datadir + '/f0/' + bname + '.f0'
            
                gcitmp = conf.datadir + '/gci/' + bname + '.GCItmp'                
                gcifile = conf.datadir + '/gci/' + bname + '.GCI'
                
                # analysis commands
                #cmd =  conf.reaper + ' -a -i ' + wavfile + ' -f ' + f0tmp1 + ' -p ' + gcitmp + ' -u 0.05'
                #cmd =  conf.reaper + ' -a -i ' + wavfile + ' -f ' + f0tmp1 + ' -p ' + gcitmp + ' -t -x 500 -m 50' # -t for Hilbert transform
                cmd =  conf.reaper + ' -a -i ' + wavfile + ' -f ' + f0tmp1 + ' -p ' + gcitmp + ' -x 500 -m 40'
                os.system(cmd)
                # todo: use conf.sopr and conf.x2x
                cmd = 'tail +8 ' + f0tmp1 + '| awk \'{print $3}\' | x2x +af ' + \
                    '| sopr -magic -1.0 -MAGIC 0.0  > ' + f0tmp2
                os.system(cmd)
                # only take voiced pitch marks
                cmd = 'tail +8 ' + gcitmp + '| awk \'$2 == "1" {print $1}\' | x2x +af > ' + gcifile
                os.system(cmd)
            
                # read the file
                f0 = np.fromfile(f0tmp2, dtype=np.float32, count=-1, sep='')
                n_frames = len(f0)

                # calculate the sptk compatible number of frames 
                wave_read = wave.open(wavfile,'r')
                n_samples = wave_read.getnframes()
                sample_rate = wave_read.getframerate()
                n_frames_target =  int(math.ceil(n_samples/(0.005*sample_rate)))
                wave_read.close
 
                # zero pad and save f0
                f0_true = np.zeros(n_frames_target, dtype= np.float32)
                npad_start = 2
                f0_true[npad_start:npad_start+n_frames] = f0
                f0_true.astype(np.float32).tofile(f0file, sep='',format="%f")

                # remove tmp
                os.remove(f0tmp1)
                os.remove(f0tmp2)
                os.remove(gcitmp)

def glott_vocoder_analysis():
    wavscp = conf.datadir + '/scp/wav.scp'
    with open(wavscp,'r') as wavfiles:
        for file in wavfiles:
            wavfile = file.rstrip()
            if os.path.isfile(wavfile):
                # define file paths
                bname = os.path.splitext(os.path.basename(wavfile))[0]
                f0file = os.path.join(conf.datadir, 'f0', bname + '.f0')
                gcifile = conf.datadir + '/gci/' + bname + '.GCI'
                # create temporary analysis config for file 
                config_user = 'config_user.cfg'
                conf_file = open(config_user,'w')
                if conf.do_sptk_pitch_analysis or conf.do_reaper_pitch_analysis:
                    conf_file.write('USE_EXTERNAL_F0 = true;\n')
                    conf_file.write('EXTERNAL_F0_FILENAME = \"' + f0file + '\";\n' )
                else:
                    conf_file.write('USE_EXTERNAL_F0 = false;\n')
                if conf.use_external_gci:
                    conf_file.write('USE_EXTERNAL_GCI = true;\n')
                    conf_file.write('EXTERNAL_GCI_FILENAME = \"' + gcifile + '\";\n' )
                conf_file.write('SAMPLING_FREQUENCY = ' + str(conf.sampling_frequency) +';\n')
                conf_file.write('WARPING_LAMBDA_VT = '+ str(conf.warping_lambda) +';\n')
                conf_file.write('DATA_DIRECTORY = \"' + conf.datadir + '\";\n')
                conf_file.write('SAVE_TO_DATADIR_ROOT = false;\n')
                # force the use of float file format
                conf_file.write('DATA_TYPE = \"FLOAT\";\n')
                conf_file.close()
                # run analysis program
                cmd = conf.Analysis + ' ' + wavfile + ' ' + conf.config_default + ' ' + config_user
                os.system(cmd)
                # remove temporary config
                os.remove(config_user)

def glott_vocoder_synthesis():
    wavscp = conf.datadir + '/scp/wav.scp'
    with open(wavscp,'r') as wavfiles:
        for f in wavfiles:
            wavfile = f.rstrip()
            if os.path.isfile(wavfile):
                bname = os.path.splitext(os.path.basename(wavfile))[0]
                f0file = conf.datadir + '/f0/' + bname + '.f0'
                config_user = 'config_user.cfg'
                conf_file = open(config_user,'w');
                conf_file.write('SAMPLING_FREQUENCY = ' + str(conf.sampling_frequency) +';\n')
                conf_file.write('WARPING_LAMBDA_VT = '+ str(conf.warping_lambda) +';\n')
                conf_file.write('DATA_DIRECTORY = \"' + conf.datadir + '\";\n')
                conf_file.write('DATA_TYPE = \"FLOAT\";\n')
                conf_file.write('SAVE_TO_DATADIR_ROOT = false;\n')

                if conf.use_dnn_generated_excitation:
                    conf_file.write('EXCITATION_METHOD = \"DNN_GENERATED\";\n')   
                conf_file.write('DNN_WEIGHT_PATH = \"' + conf.weights_data_dir + '/' + conf.dnn_name + '\";\n')
                
                conf_file.close()
                cmd = conf.Synthesis + ' ' + wavfile + ' ' + conf.config_default + ' ' + config_user
                #print cmd
                os.system(cmd)


def package_data():
    # read and shuffle wav filelist
    wavscp = conf.datadir + '/scp/wav.scp' 
    with open(wavscp,'r') as wavfiles:
        filelist = wavfiles.read().splitlines()    
    random.shuffle(filelist)
    
    if conf.max_number_of_files < len(filelist):
        filelist = filelist[0:conf.max_number_of_files]

    # initialize global min and max
    in_min = 9999*np.ones([1,sum(conf.input_dims)],dtype=np.float32)
    in_max = -9999*np.ones([1,sum(conf.input_dims)],dtype=np.float32)    
    
    n_frames = np.zeros([len(filelist)], dtype='int')
    for file_idx, wavfile in enumerate(filelist):
        if os.path.isfile(wavfile):
            bname = os.path.splitext(os.path.basename(wavfile))[0]
            print (bname)
                # todo: save n_frames
            f0_file = conf.datadir + '/f0/' + bname + '.f0' 
            n_frames[file_idx] = (np.fromfile(f0_file, dtype=np.float32, count=-1, sep='')).shape[0]
            # allocate file data
            input_data = np.empty([n_frames[file_idx], sum(conf.input_dims)], dtype=np.float32)
            feat_start = 0
            for (ftype, ext, dim) in zip( conf.inputs, conf.input_exts, conf.input_dims):
                if dim > 0:
                    # read feat  data
                    feat_file = conf.datadir + '/'+ ftype + '/' + bname + ext
                    feat = np.fromfile(feat_file, dtype=np.float32, count=-1, sep='')
                    # check length is multiple of feature dimension
                    assert len(feat) % dim == 0, \
                        " Length mismatch for " + ftype
                    # reshape
                    feat = np.reshape(feat, (-1,dim))
                    # set to input data matrix
                    print(feat_file)
                    print(feat.shape)
                    try:
                        input_data[:,feat_start:feat_start+dim ] = feat
                    except ValueError as e:
                        print("Error reading " + feat_file)
                        print("Check that the feature sizes match between vocoder and python configs")
                        raise e

                    feat_start += dim
            # remove unvoiced frames if requested
            if conf.remove_unvoiced_frames:
                input_data = input_data[input_data[:,0] > 0,:]
            # update global min and max    
            in_min = np.minimum(np.amin(input_data, axis=0), in_min)
            in_max = np.maximum(np.amax(input_data, axis=0), in_max)

    new_min = 0.1
    new_max = 0.9
    
    n_val = round(conf.validation_ratio * len(filelist))
    n_test = round(conf.test_ratio * len(filelist))
    n_train = len(filelist) - n_val - n_test
    if n_train < 0:
        print ("oops")
    set_name = ['train', 'val', 'test']
    set_sizes = [n_train , n_val, n_test]
    print (set_sizes)

#    batch_index = 1
#    in_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + str(batch_index) + '.idat' ,'w')
#    out_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + str(batch_index) + '.odat' ,'w')

    set_file_counter = 1
    set_index = 0
    in_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + set_name[set_index] + '.idat' ,'w')
    out_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + set_name[set_index] + '.odat' ,'w')
                        
    for file_idx, wavfile in enumerate(filelist):

        if set_file_counter > set_sizes[set_index]:
            set_file_counter = 1
            set_index += 1
            in_fid.close()
            out_fid.close()            
            if set_sizes[set_index] == 0:
                set_index += 1
                continue
            else:
                in_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + set_name[set_index] + '.idat' ,'w')
                out_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + set_name[set_index] + '.odat' ,'w')
            

        #if file_idx > 0 and file_idx % conf.data_buffer_size == 0:
        #    in_fid.close()
        #    out_fid.close()
        #    batch_index += 1
        #    in_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + str(batch_index) + '.idat' ,'w')
        #    out_fid = open(conf.train_data_dir + '/' + conf.dnn_name + '.' + str(batch_index) + '.odat' ,'w')
            
   
        if os.path.isfile(wavfile):
            bname = os.path.splitext(os.path.basename(wavfile))[0]                
            # allocate input and output data
            input_data = np.empty([n_frames[file_idx], sum(conf.input_dims)], dtype=np.float32)
            output_data = np.empty([n_frames[file_idx], sum(conf.output_dims)], dtype=np.float32)
   
            # read input data
            feat_start = 0
            for (ftype, ext, dim) in zip( conf.inputs, conf.input_exts, conf.input_dims):
                if dim > 0:
                    feat_file = conf.datadir + '/'+ ftype + '/' + bname + ext
                    feat = np.fromfile(feat_file, dtype=np.float32, count=-1, sep='')
                    feat = np.reshape(feat, (-1,dim))
                    input_data[:,feat_start:feat_start+dim ] = feat
                    feat_start += dim

            # read output data
            feat_start = 0
            for (ftype, ext, dim) in zip( conf.outputs, conf.output_exts, conf.output_dims): 
                if dim > 0:
                    feat_file = conf.datadir + '/'+ ftype + '/' + bname + ext
                    feat = np.fromfile(feat_file, dtype=np.float32, count=-1, sep='')
                    feat = np.reshape(feat, (-1,dim))
                    output_data[:,feat_start:feat_start+dim ] = feat
                    feat_start += dim
            
            # remove unvoiced frames if requested
            if conf.remove_unvoiced_frames:
                output_data = output_data[input_data[:,0] > 0,:]
                input_data = input_data[input_data[:,0] > 0,:]
            
            # normalize and write input data
            input_data = (input_data - in_min) / (in_max - in_min) * (new_max - new_min) + new_min
            input_data.astype(np.float32).tofile(in_fid, sep='',format="%f")
            
            # write output data
            output_data.astype(np.float32).tofile(out_fid, sep='',format="%f")

            set_file_counter += 1
            
    # close files
    in_fid.close()
    out_fid.close()

    # write input min and max
    fid = open(conf.weights_data_dir + '/' + conf.dnn_name + '.dnnMinMax' ,'w')
    in_min.astype(np.float32).tofile(fid, sep='', format="%f")
    in_max.astype(np.float32).tofile(fid, sep='', format="%f")
    fid.close()
    
def make_data_provider(conf):

    dataset = dutil.Dataset()

    # add inputs to dataset
    for key, ext, dim in zip(conf.inputs, conf.input_exts, conf.input_dims):
        scp = conf.datadir + '/scp/' + key + '.scp'
        dobj = dutil.DataObj(key, dim, scp, ext)   
        dataset.addDataObj(dobj, scaler='minmax')
        
    # add outputs to dataset    
    for key, ext, dim in zip(conf.outputs, conf.output_exts, conf.output_dims):
        scp = conf.datadir + '/scp/' + key + '.scp'
        dobj = dutil.DataObj(key, dim, scp, ext)
        dataset.addDataObj(dobj, scaler=None)
     
    # compute and save dataset scaler statistics
    dataset.compute_stats()
    
    # save dataset provider (no data is copied)
    dataset.saveDataProvider('./data_provider.dat')
    
    
    #x, y = dataset.getDataInputOutput(0, ['lsf','f0'], ['pls'])
    #print x.shape
    #print x
    #print y.shape
  
    """
      
    for data in Dataset_iter(dataset, shuffle=False, io_keys=[conf.inputs, conf.outputs]):
        print data.tag
        print data.inputs.shape
        print data.outputs.shape
        
    
        
    for data_batch in DatasetBatchBuffer_iter(dataset, shuffle=False, batch_size=128, io_keys=[conf.inputs, conf.outputs]):
        print data_batch[0].shape
        print data_batch[1].shape
    
    """
    
    print (dataset.getFileTag())
    
    ktrain.train_model(dataset, batch_size=128,
                input_keys=conf.inputs, output_keys=conf.outputs)
    
    ktrain.generate(dataset,
                input_keys=conf.inputs, output_keys=conf.outputs,
                out_dir=conf.gen_out_dir)
    
    # test for training 
    #for data in Dataset_iter(dataset, shuffle=True):
    #    print data['pls'].shape
    
def train_model_keras(conf):
    
    print ("not implemented yet")

def main(argv):
   
    # Make directories
    if conf.make_dirs:
        make_directories()

    # Make SCP list for wav
    if conf.make_scp:
        make_file_lists()

    # Reaper pitch estimation
    if conf.do_reaper_pitch_analysis:
        reaper_pitch_analysis()
       
    # SPTK pitch estimation
    if conf.do_sptk_pitch_analysis:
        sptk_pitch_analysis()
    
    # GlottDNN Analysis
    if conf.do_glott_vocoder_analysis:
        glott_vocoder_analysis()

    # Package data for Theano
    if conf.make_dnn_train_data:
        package_data()
        
    # Make data provider for generic excitation model training
    try:
        if conf.make_data_provider:
            make_data_provider(conf)
    except AttributeError:
        warnings.warn("Config file does not have attribute 'make_data_provider'" )

    # Write Dnn infofile
    if conf.make_dnn_infofile:
        write_dnn_infofile()

    # Train Dnn with Theano
    if conf.do_dnn_training:
        dim_in = sum(conf.input_dims)
        dim_out = sum(conf.output_dims)
        TrainDnn.evaluate_dnn(n_in=dim_in, n_out=dim_out, n_hidden=conf.n_hidden, batch_size=conf.batch_size, 
                 learning_rate=conf.learning_rate, n_epochs = conf.max_epochs)

    # Copy-synthesis
    if conf.do_glott_vocoder_synthesis:
        glott_vocoder_synthesis()
    
if __name__ == "__main__":
    main(sys.argv[1:])
