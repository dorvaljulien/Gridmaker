; This function takes a number of seconds and turns it into a "readable" string 
function print_time,time_seconds
seconds=time_seconds
minutes=floor(seconds)/60
hours=minutes/60
days=hours/24

swtch=1

if days ne 0 && swtch eq 1 then begin
        if days gt 15 then return, strtrim(days,2)+' days'
        hours=hours-24*days
        return,strtrim(days,2)+' d '+strtrim(hours,2)+' h'
        swtch=0
endif

if hours ne 0 && swtch eq 1 then begin
        minutes=minutes-60*hours
        return,strtrim(hours,2)+' h '+strtrim(minutes,2)+' m'
        swtch=0
endif

if minutes ne 0 && swtch eq 1 then begin
        seconds=seconds-60*minutes
        return,strtrim(minutes,2)+' m '+strtrim(string(seconds,format='(I)'),2)+' s'
        swtch=0
endif

if minutes eq 0 && swtch eq 1 then begin
        return,strtrim(string(seconds,format='(F5.2)'),2)+' s'
        swtch=0
endif

return,datestring
end

;This turns '1.50' into '0150'
function format_4digits, string 
str_parts = STRSPLIT(string, '.', /EXTRACT)  
return, STRJOIN(['0',str_parts],'')  
end

;This turns '1.50' into '150'
function format_3digits, string 
str_parts = STRSPLIT(string, '.', /EXTRACT)  
return, STRJOIN([str_parts],'')  
end

;This function takes the global n index and returns the index of the corresponding metallicity
function n_z, n,n_masses,n_length
n_z=n/(n_masses*n_length)
return,n_z
end

;This function takes the global n index and returns the index of the corresponding mass
function n_m,n,n_masses,n_length
n_m=(n-n_z(n,n_masses,n_length)*(n_length*n_masses))/n_length
return,n_m
end

;This function takes the global n index and returns the index of the corresponding mixing length
function n_l, n,n_masses,n_length
n_l=n-n_z(n,n_masses,n_length)*(n_length*n_masses)-n_m(n,n_masses,n_length)*n_length
return,n_l
end

;This just returns the specified number of blank spaces
function blankstring,s
if s lt 0 then return,'negative blankstring argument'
blank=''
for i=1,s do blank=blank+' '
return, blank
end

;----------------------------------------------------------------------------------------------------------

;This function returns the number of stars completed by the code in a given metallicity
function progression_z,execution_times,repartition,nz,n_masses,n_length
metal_number=n_z(repartition,n_masses,n_length)
index=where(metal_number eq nz)
if index(0) eq -1 then begin
        if nz lt min(metal_number) then progression=n_length*n_masses
        if nz gt max(metal_number) then progression=0
        if nz gt min(metal_number) && nz lt max(metal_number) then progression=n_length*n_masses
endif else begin
        index_time=where(execution_times(nz,*) ne 0)
        if index_time(0) eq -1 then begin
                progression=0
        endif else begin
                progression=n_elements(index_time)
        endelse
endelse

if progression lt 0 then begin
        print,'Stop ! Negative progression, there is a problem in progression_z !'
        stop
endif

return,progression

end

;----------------------------------------------------------------------------------------------------------

;This function prepare the zams and the inlist file for a MESA run, then launch it.
;Enabling the /sleep keyword allows to launch a sleep command with a random argument
;instead of MESA. This is useful for debugging grid_creator or resume_grid.
pro launch_tube,i,n,n1,n2,path,z,m,l,n_core,zams_path,SLEEP=sleep

           
m_string=m(n_m(n,n1,n2))
z_string=z(n_z(n,n1,n2))
l_string=L(n_l(n,n1,n2)) 
l_format=format_3digits(l_string) 
m_format=format_4digits(m_string)


         
;Create the variable part of the inlist
openw,1,'tube'+strtrim(i,2)+'/inlist_parameters'
        printf,1,'&controls'
        printf,1,''
        printf,1,'zams_filename='+"'"+zams_path+'/zams_z'+z_string+'_dM0.01_M0.5-5.0.data'+"'"
        printf,1,'initial_Z ='+z_string
        printf,1,'initial_mass ='+m_string+' ! in Msun units'
        printf,1,'mixing_length_alpha ='+l_string
        printf,1,'profile_data_prefix='+"'m"+m_format+'.z'+z_string+'.a'+l_format+'.o000.'+"'"
        printf,1,''
        printf,1,'/'
close,1

print,systime()+': launching Z '+z_string+'  M '+m_string+'  L '+l_string+' on tube '+strtrim(i,2)

;Launch MESA and create the 'free' file flag when it's done
if keyword_set(sleep) then begin 
        spawn,'cd '+path+'/tube'+strtrim(i,2)+';sleep '+strtrim(floor(40*RANDOMU(seed,1)),2)+'; touch LOGS/profile_temoin_'+strtrim(n,2)+' ;touch free &'
        print,'sleep'
endif else begin
        spawn,'cd '+path+'/tube'+strtrim(i,2)+'; setenv OMP_NUM_THREADS '+strtrim(n_core,2)+';  ./rn > logstorage ;touch free &'
endelse

end

;----------------------------------------------------------------------------------------------------------

;This function returns the number of stars being processed at the time of call in the specified metallicity
function n_processing, nz,repartition, n1, n2

nz_array=n_z(repartition,n1,n2)
array_n_processing=where(nz_array eq nz)

if min(array_n_processing) eq -1 then begin 
        tmp_n_processing=0
endif else begin
        tmp_n_processing=n_elements(array_n_processing)
endelse

return, tmp_n_processing
end

;----------------------------------------------------------------------------------------------------------

;This function prints all the new needed information in the header of the progress file.
pro update_header, repartition, execution_times, t_total_s, n_tubes, n_total 
tab=string(9b)+string(9b)
                            
completed_array = where( execution_times ne 0 )
if completed_array(0) eq -1 then begin 
n_completed=0
endif else begin
n_completed=n_elements(completed_array)
endelse

    
if n_completed eq 0  then begin
        progression=0
        t_remaining=' --- '
        mean_time=0
endif else begin
        progression=float(n_completed)/n_total
        mean_time=mean(execution_times(completed_array))
        remaining_estimate=(n_total -n_completed)*mean_time
        t_remaining=print_time(remaining_estimate/n_tubes)
endelse
    
t_total=print_time(t_total_s)
          
progression=strtrim(string(100.*progression,format='(F6.2)')+'%',2) 
          
openw,1,'progress/header.part'
        printf,1,progression+' overall progression, '+t_total+' since starting point,'+t_remaining+' estimated remaining time (+/- 1 or 2 days)    global mean:'+print_time(mean_time)
        printf,1,''
        printf,1,'metallicity'+blankstring(9)$
                +tab+'progression'+blankstring(19)+tab+'min ex. time'+blankstring(2)$
                +tab+'max ex. time'+blankstring(2)+tab+'average
close,1   
          
end

;----------------------------------------------------------------------------------------------------------

;This function prints all the needed new information in the mettalicity part of the progress file
pro update_z_part,nz,repartition,progression, prefix,z,suffix, execution_times, n1,n2 
tab=string(9b)+string(9b)     

z_execution_times=execution_times(nz,*)
n_totalz=strtrim(n1*n2,2)   
                 
n_current=strtrim(n_processing(nz,repartition,n1,n2),2)
             
local_progression=progression+' / '+n_totalz+' computed'
if n_current ne 0 THEN local_progression= local_progression+' - '+n_current+' running'

completed_array = where( z_execution_times ne 0 )
if completed_array(0) eq -1 then begin 
n_completed=0
endif else begin
n_completed=n_elements(completed_array)
endelse

if n_completed eq 0 then begin
local_min='0'
local_max='0'
local_mean='0'
endif else begin
local_min=print_time(min(z_execution_times(completed_array)))
local_max=print_time(max(z_execution_times(completed_array)))
local_mean=print_time(mean(z_execution_times(completed_array)))
endelse

openw,1,'progress/'+prefix+z(nz)+suffix                         
        printf,1,z(nz)+blankstring(20-strlen(z(nz)))+tab+local_progression+blankstring(30-strlen(local_progression))+tab+$
                local_min+blankstring(15-strlen(local_min))+tab+$
                local_max+blankstring(15-strlen(local_max))+tab+$
                local_mean+blankstring(15-strlen(local_mean))
close,1
end

;----------------------------------------------------------------------------------------------------------

;This function writes the previous state of the grid progress and information about the interruption into the
;z_0.0000000.part which will be concatenated with real metallicity .part files to form the progression file.
pro write_interruption_info, repartition, completed, execution_times, interruption_date, path, z, n1, n2
tab=string(9b)+string(9b) 

n_totalz=strtrim(n1*n2,2)
nz_array=n_z(repartition,n1,n2)
n_max_current_z=max(nz_array)

for i=0,n_max_current_z do begin
        progression=strtrim(progression_z(execution_times,repartition,i,n1,n2),2)
        update_z_part,i,repartition,progression, "",z,".interruption", execution_times, n1,n2 
endfor

openw,1,"progress/message.interruption"     
        ;Now the interruption message
        printf,1,'--------------INTERRUPTED-------------------------------------------------------------------------------------------------------------------------'
        printf,1,'Date of interruption: '+interruption_date
        printf,1,'Resumed on:           '+systime()
        printf,1,'---------------RESUMING---------------------------------------------------------------------------------------------------------------------------'
close,1

spawn, 'cd '+path+'/progress; cat *.interruption >> z_0.0000000.part; rm *.interruption'

;Concatenate these to create the progress file
spawn,'cd '+path+'/progress; cat *.part > progression;'

end
