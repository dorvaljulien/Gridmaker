file_id = H5F_OPEN('hdf5_processing/test.h5')

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Accessing a history file:

;Accessing a history file
aset_id = H5D_OPEN(file_id, '0.0010000/M0.50_L1.80/history/age')
age= H5D_READ(aset_id)
H5D_CLOSE, aset_id

aset_id = H5D_OPEN(file_id, '0.0010000/M0.50_L1.80/history/log_Teff')
log_Teff= H5D_READ(aset_id)
H5D_CLOSE, aset_id

;Acessing the number of model for this star
group= H5G_OPEN(file_id,'0.0010000/M0.50_L1.80/history/')
attribute = H5A_OPEN_NAME(group, 'maxmodel')
 maxmodel  = H5A_read(attribute)
H5A_CLOSE, attribute
H5G_close, group


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Accessing a profile:

; Access the zone numbers for the 15th model of Z0.0010000 M0.50 L1.80
aset_id = H5D_OPEN(file_id, '0.0010000/M0.50_L1.80/15/logP')
logP = H5D_READ(aset_id)
H5D_CLOSE, aset_id

; Access logRho for the same model
aset_id = H5D_OPEN(file_id, '0.0010000/M0.50_L1.80/15/logT')
logT = H5D_READ(aset_id)
H5D_CLOSE, aset_id

;Access the number of zones in this model
group= H5G_OPEN(file_id,'0.0010000/M0.50_L1.80/15/')
attribute = H5A_OPEN_NAME(group, 'nzones')
 nzones  = H5A_read(attribute)
H5A_CLOSE, attribute
H5G_close, group


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;This is a way to store the structure of the hdf5 file into an IDL structure.
;But it only works for very small ones, one or two stars around 0.5M. 

;res = H5_PARSE('hdf5_processing/test.h5')

H5F_close,file_id


end