@functions.pro


pro resume_grid, parameters_file, CHECK_FLAGS=check_flags

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
hdf5_path=path+'/hdf5_processing/


;We check IDL is working in the directory pointed by global_path
spawn,'touch '+path+'/TEST'
if file_test('TEST') eq 0 then begin
        spawn,'rm '+path+'/TEST'
        print,'The IDL working directory is not the one defined in parameters.txt'
        goto, end_of_procedure
endif
spawn,'rm '+path+'/TEST'

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


;Read the metallicities, masses and mixinglengths
readcol,z_list,z,format='(A32)',/silent
readcol,mass_list,m,format='(A32)',/silent
readcol,length_list,L,format='(A32)',/silent

;Get the repartition from when it was interrupted
readcol,'repartition',repartition,format='(I)',/silent
old_n_tubes=n_elements(repartition)

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

;Tab symbol, useful to arrange the progress file
tab=string(9b)+string(9b)

max_current_z=max(n_z(repartition,n1,n2))
min_current_z=min(n_z(repartition,n1,n2))

completed=intarr(n_metal)

;execution time arrays
execution_times=dblarr(n_metal,n_length*n_masses)
times=dblarr(n_tubes)

;We read the execution times files
for i=0,max_current_z do begin
        readcol,'execution_times/times_'+z(i),tmp,/silent
        execution_times(i,*)=tmp
        if min(execution_times(i,*)) ne 0 then completed(i)=1
endfor

; If none of the execution times is 0, the grid is complete.
if min(execution_times) ne 0 then begin
        print,"Error, the current grid is complete. Use grid_creator to begin a new one."
        goto, end_of_procedure;
endif

;Check that we won't run out of cores
if n_core*n_tubes gt 38 then begin
        print,'Too many cores needed for extend, choose fewer tubes or less cores'
        goto, end_of_procedure;
endif


if keyword_set(check_flags) then begin
        ;We check all the processes from previous run ended: there should be a free flag everywhere
        for i=0,n_tubes-1 do begin
        if file_test('tube'+strtrim(i+1,2)+'/free') ne 1 then begin
                print,'No free flag in tube'+strtrim(i+1,2)+', please check that there is no remaining process and try again'
                goto, end_of_procedure
        endif
        endfor
endif

;----------------------------------------------------------------------------------------------------------------------------
print,'Stopping remaining processes'
spawn,'cd '+hdf5_path+'; ./stop; '
print,'Creating the new tubes if needed' 
spawn,'cd '+path+'; ./tube_copier '+strtrim(n_tubes,2)
print,'Launching back ./output_data_deletion and ./hdf5_maker'
spawn,'cd '+hdf5_path+'; ./output_data_deletion &'
spawn,'cd '+hdf5_path+'; ./hdf5_maker '+output_path+' test.h5 &'
;----------------------------------------------------------------------------------------------------------------------------


interruption_date=''; This is needed to tell IDL it has to read a string
openr,1,'journal'
        ;date while interrupted
        readf,1,interruption_date
        ;elapsed time in seconds in the past simulations
        readf,1,elapsed_time_s
close,1

;New time origin
t_origin=systime(/seconds)

;-----------------------------------------------------------------------------------------------------
;We update the progress file by writing information about the interruption
write_interruption_info, repartition, completed, execution_times, interruption_date, path, z, n1, n2
;-----------------------------------------------------------------------------------------------------
    
;sort the repartition to make it easier:
repartition=repartition(sort(repartition))
     

;Launch the first tubes
for i=0,n_tubes-1 do begin
          
        ;Clean the LOGS directory (might take a while)
        print,'cleaning the logs on tube'+strtrim(i+1,2)+' ... '
        spawn,'rm '+path+'/tube'+strtrim(i+1,2)+'/LOGS/*'
          
        ;We resume the last repartition, while dealing with a possible changed number of tubes
        if i lt old_n_tubes then n=repartition(i)
        if i ge old_n_tubes then n=n+1
         
        ;We launch the tube i+1  
        launch_tube,i+1,n,n1,n2,path,z,m,l,1,zams_path  
        times(i)=systime(/seconds)
        
        ;Removing the free flag
        spawn,'cd '+path+';./rm_silent ./tube'+strtrim(i+1,2)+'/free'
         
endfor

;Since we sorted the repartition, the last tube we launched had the maximum n in the repartition
;We can resume the normal flow of the program in the main loop by setting the n of the next star
;to be computed:
n++

;We now adjust the size of the repartition array to the new number of tubes
if n_tubes gt old_n_tubes then begin
        while n_elements(repartition) ne n_tubes do repartition=[repartition,max(repartition)+1]
endif

if n_tubes lt old_n_tubes then repartition=repartition(0:n_tubes-1)


;Entering the main loop, watching the tubes for the presence of the free flag, signaling
;the current job is finished and that a new model can be launched. The global switch remains
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
                                          
                        ;Record the execution time
                        execution_times(n_z(repartition(i),n1,n2),repartition(i)-n_z(repartition(i),n1,n2)*n_length*n_masses)=systime(/seconds)-times(i)
                                          
                        ;Rename the FGONG files:
                        spawn,'cd '+path+'; ./fgong_renamer tube'+strtrim(i+1,2)+'/LOGS'                  
                                          
                        ;export the data from the tube directory. make_or_replace will delete any existing
                        ;directory with the same name than the current star. If there is one, it is probably
                        ;data that was being copied when the grid stopped.
                        spawn,'cd '+path+'; ./make_or_replace '+output_path+'/'+z_string+'/M'+mass_string+'_L'+l_string+$
                                '; mv tube'+strtrim(i+1,2)+'/LOGS/* '+output_path+'/'+z_string+'/M'+mass_string+'_L'+l_string+'/;'
                        
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
                t_total_s=systime(/seconds)-t_origin+elapsed_time_s            
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
                printf,1,systime(/seconds)-t_origin+elapsed_time_s 
        close,1

        ;If we're done computing every model, exit the loop
        if min(repartition) eq n_total then global_switch=1
endwhile

spawn,'cd '+hdf5_path+'; echo END >> todolist'
spawn,'cd '+path+'; ./tube_deleter'

end_of_procedure:
end