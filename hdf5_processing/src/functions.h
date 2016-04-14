#include "hdf5.h"
int file_exists(char filename[]);
int file_doesnt_exist(char filename[]);
void update_log( char new_content[] , char logfilename[] );
double** make2Ddoublearray(int arraySizeX, int arraySizeY);
void free2Ddoublearray( double **array, int nrow);
void free_string_array( char **array, int nstring);
int getnames(char filename[],int skiplines, char ***outarray, int *nnames, char logfilename[]);
int getdata(char filename[], int skiplines, int ncol, int *nlines, double ***dataarray, char logfilename[]);
void get_file_path(char* buffer[], char source_path[], char z[], char m[],char L[]);
void get_file_name(char* buffer[], int file_number, int type_choice, char file_path[], char z[], char m[],char L[]);
int get_last_model(char filename[]);
int get_nzones(char filename[]);
void writehdf5(hid_t place_id, char **columns_name_array_profile, double **data_array_profile, int ncol, int nline, int scalar_attribute, char attribute_name[]);
int star_is_invalid(char source_path[],char z[],char m[], char l[], int *inc, char *error_message[],char logfilename[]);
int process_to_hdf5(char source_path[], char hdffilename[],char z[],char m[],char l[], char logfilename[]);
char waitfor(char filename[], FILE* file, char ch1, char ch2, char ch3, char logfilename[]);
void read_parameters(FILE* file, char* z, char* m, char* l);
int read_star_count(char star_count_filename[]);
void update_star_count(char star_count_filename[],int star_count);
void delete_output_data(char source_path[] , char* z , char* m , char* l );




