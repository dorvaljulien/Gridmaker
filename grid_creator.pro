@functions.pro

pro grid_creator, parameters_file, OVERWRITE=overwrite

;We close any file still open
close, /all



;Check the existence of the parameters file
if file_test(parameters_file) eq 0 then begin
        print,parameters_file+' not found'
        goto, end_of_procedure;
endif


starting_parameters=''
for i=0,10 do starting_parameters=[starting_parameters,'']
readcol,parameters_file,starting_parameters,format='(A)',/silent

z_list=starting_parameters[0]
mass_list=starting_parameters[1]
length_list=starting_parameters[2]
n_tubes=floor(float(starting_parameters[3]))
n_core=floor(float(starting_parameters[4]))
path=starting_parameters[5]
output_path=starting_parameters[6]
zams_path=starting_parameters[7]
hdf5_filename=starting_parameters[8]
hdf5_path=path+'/hdf5_processing/'


;We check IDL is working in the directory pointed by global_path
spawn,'touch '+path+'/TEST'
if file_test('TEST') eq 0 then begin
        spawn,'rm '+path+'/TEST'
        print,'The IDL working directory is not the one defined in parameters.txt'
        goto, end_of_procedure
endif
spawn,'rm '+path+'/TEST'

;Check that we won't run out of cores
if n_core*n_tubes gt 40 then begin
        print,'Too many cores needed for extend, choose fewer tubes or less cores'
        goto, end_of_procedure;
endif

;Check the existence of the .list files:
if file_test(z_list) eq 0 then begin
        print,z_list+' not found'
        goto, end_of_procedure;
endif
if file_test(mass_list) eq 0 then begin
        print,mass_list+' not found'
        goto, end_of_procedure;
endif
if file_test(length_list) eq 0 then begin
        print,length_list+' not found'
        goto, end_of_procedure;
endif



;Clean all the files created from previous run
spawn,'cd '+path+'; ./cleangrid'
;Create the output directory if it is not already there
spawn,'cd '+path+'; ./make_directory '+output_path
;Create all the needed tubes, if they are not already there
spawn,'cd '+path+'; ./tube_copier '+strtrim(n_tubes,2)
;Copy the shared_inlist in each tube
spawn,'cd '+path+'; ./inlist_copier '+strtrim(n_tubes,2)


;Read the metallicities, masses and mixinglengths
readcol,z_list,z,format='(A32)',/silent
readcol,mass_list,m,format='(A32)',/silent
readcol,length_list,L,format='(A32)',/silent

;Sizes of the arrays we are going to use
n_length=n_elements(L)
n_metal=n_elements(z)
n_masses=n_elements(m)
;to make it more compact when calling the n_z, n_m, n_l
n1=n_masses
n2=n_length

;Total number of models to compute
n_total=n_metal*n_masses*n_length
;Number of models per metallicity
n_totalz=strtrim(n_masses*n_length,2)

;If we have more tubes than models, reduce it.
if n_total lt n_tubes then n_tubes=n_total

;Initialize the incrementation of models
n=0

;Tab symbol, useful to arrange the progress file
tab=string(9b)+string(9b)

;Repartition array: contains the models number being computed in each tube
repartition=intarr(n_tubes)

;execution time arrays
execution_times=dblarr(n_metal,n_length*n_masses)
times=dblarr(n_tubes)
completed=intarr(n_metal) ; For each metallicity, 1=all stars completed, 0 otherwise

;Time origin for computations
t_origin=systime(/seconds)


print,'Reseting the processing folder and launching the making and deletion processes'
;-------------------------------------------------------------------------------------------
spawn,'cd '+hdf5_path+';./stop;'

if keyword_set(overwrite) then begin
        spawn,'cd '+hdf5_path+'; ./reset_overwrite'
endif else begin
        spawn,'cd '+hdf5_path+';./reset'
endelse

spawn,'cd '+hdf5_path+'; ./output_data_deletion &'
spawn,'cd '+hdf5_path+'; ./hdf5_maker '+output_path+' '+hdf5_filename+' &'
;-------------------------------------------------------------------------------------------




;Create all the .parts which constitute the progress file:
;One header  + one line per metallicity
;Create the header
openw,1,'progress/header.part'
	printf,1,'0% overall progression, 0s since starting point, no estimation of remaining time yet'
	printf,1,''
	printf,1,'metallicity'+blankstring(9)$
        	+tab+'progression'+blankstring(19)+tab+'min ex. time'+blankstring(2)$
        	+tab+'max ex. time'+blankstring(2)+tab+'average'
close,1

;Create the metallicity files
for i=0,n_metal-1 do begin
        openw,1,'progress/z_'+z(i)+'.part'
        printf,1,z(i)+blankstring(20-strlen(z(i)))$
                +tab+'-'+blankstring(29)+tab+'0'+blankstring(14)+tab+'0'+blankstring(14)+tab+'0'        
        close,1
endfor

;Concatenate these to create the progress file
spawn,'cd '+path+'/progress; cat *.part > progression;'

;Let's create the output directories
for i=0,n_metal-1 do begin
        spawn,'cd '+output_path+'; mkdir '+z(i)
endfor


;Launch the first tubes
for i=0,n_tubes-1 do begin
         launch_tube,i+1,n,n1,n2,path,z,m,l,n_core,zams_path
         times(i)=systime(/seconds)
         repartition(i)=n
         n++  
endfor

;Let's write down the repartition on a txt file
openw,1,'repartition'
        for i=0,n_tubes-1 do   printf,1,repartition(i)
close,1


;Entering the main loop, watching the tubes for the presence of the free flag, signaling
;the current job is finished and a new model can be launched. The global switch remains
;Turned off as long as not all models are done computing
global_switch=0
while global_switch eq 0 do begin

        for i=0,n_tubes-1 do begin
     
                state=file_test('tube'+strtrim(i+1,2)+'/free')
                ;if state equals one, the current tube is free. We can export
                ;the data and prepare to launch the next job:
                        
                if state eq 1 then begin
                        ;we translate the current n into mass and metallicities string
                        ;The n which just ended can be obtained by taking the matching entry in the repartition array
                        mass_string=m(n_m(repartition(i),n1,n2))
                        z_string=z(n_z(repartition(i),n1,n2))
                        l_string=L(n_l(repartition(i),n1,n2))
                            
                        ; We check the existence of the history file. If it is not there, MESA probably could not start at all    
                        if file_test('tube'+strtrim(i+1,2)+'/LOGS/history.data') eq 0 then begin
                                print,'Error: MESA could not start on tube'+strtrim(i+1,2)
                                spawn,'cd '+hdf5_path+'; ./stop'
                                goto, end_of_procedure
                        endif
                            
                                          
                        ;Record the execution time
                        execution_times(n_z(repartition(i),n1,n2),repartition(i)-n_z(repartition(i),n1,n2)*n_length*n_masses)=systime(/seconds)-times(i)
                                  
                        ;Rename the FGONG files:
                        spawn,'cd '+path+'; ./fgong_renamer tube'+strtrim(i+1,2)+'/LOGS'
                        
                        ;export the data from the tube directory
                        spawn,'cd '+path+'; ./make_or_replace output/'+z_string+'/M'+mass_string+'_L'+l_string+$
                                '; mv tube'+strtrim(i+1,2)+'/LOGS/* output/'+z_string+'/M'+mass_string+'_L'+l_string+'/;'
                        
                        ;Signals the HDF5 process a star is ready in the output directory
                        spawn, 'cd '+hdf5_path+'; echo Z'+z_string+' M'+mass_string+' L'+l_string+' >> todolist'
               
                         ;Delete the  free flag
                         spawn,'rm '+path+'/tube'+strtrim(i+1,2)+'/free'
                                       
                        ;If we don't have any more to model to compute, we shut down the tube which just got free 
                        ;By attributing n_total as its job number in repartition. (The maximum valid job number is n_total-1)
                        if n gt n_total -1 then begin
                                repartition(i)=n_total
                        endif else begin
                                launch_tube,i+1,n,n1,n2,path,z,m,l,n_core,zams_path    
                                times(i)=systime(/seconds)
                                ;Update the repartition array and the repartition file                              
                                repartition(i)=n 
                                openw,1,'repartition'
                                for j=0,n_tubes-1 do  printf,1,repartition(j)
                                close,1
                        endelse   
                        
                        n++                                
                endif 
               
                       
                ;Update the progress file
                t_total_s=systime(/seconds)-t_origin            
                ;First update the header with the current progress           
                update_header, repartition, execution_times, t_total_s, n_tubes, n_total   
                        
                ;Then update the execution times and the metallicity parts of the progress file
                for j=0,n_metal-1 do begin
          
                        ;We print the executions times if we didn't already print the completed array for a given metallicity:
                        if completed(j) eq 0 then begin 
                                openw,1,'execution_times/times_'+z(j)
                                for s=0,n_length*n_masses-1 do begin
                                        printf,1,execution_times(j,s)
                                endfor
                                close,1
                        endif
   
                        ;We get the number of masses computed in the current metallicity thanks to a function coded in functions.pro
                        n_done=progression_z(execution_times,repartition,j,n_masses,n_length)
                        n_current=n_processing(j,repartition,n1,n2)
                        progression=strtrim(n_done,2)
    
                        if  n_current ne 0 || ( completed(j) eq 0 && min(execution_times(j,*)) ne 0) then begin
                                ;If we started this metallicity and we're still computing it
                                update_z_part,j,repartition,progression, "z_",z,".part", execution_times, n1,n2                 
                        endif
              
                        ;If the metallicity we're printing is actually complete, we signal that there's no need to print it again.
                        if min(execution_times(j,*)) ne 0 then completed(j)=1    
                endfor                    
                    
                ;Concatenate these to create the progress file
                spawn,'cd '+path+'/progress; cat *.part > progression;' 
              
        endfor
  
        ;Print various information about the run 
        openw,1,'journal'
        ;Current date
        printf,1,systime()
        ;elapsed time in seconds
        printf,1,t_total_s
        close,1

        ;If we're done computing every model, exit the loop
        if min(repartition) eq n_total then global_switch=1

endwhile


spawn,'cd '+hdf5_path+'; echo END >> todolist' ; This is the normal way to terminate the maker and deleter
spawn,'cd '+path+'; ./tube_deleter'
end_of_procedure:
end






