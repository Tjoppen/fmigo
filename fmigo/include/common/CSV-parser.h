#include <map>
#include <string>
#include <vector>

typedef std::vector<std::string> fmigo_csv_matrix_keys;
typedef std::map<std::string, double> fmigo_csv_value;
typedef std::map<std::string, fmigo_csv_value> fmigo_csv_map_matrix;
typedef struct fmigo_csv_matrix{
  fmigo_csv_map_matrix matrix;
  fmigo_csv_matrix_keys headers;
  fmigo_csv_matrix_keys time;
}fmigo_csv_matrix;
typedef std::map<std::string, fmigo_csv_matrix> fmigo_csv_fmu;

fmigo_csv_matrix fmigo_CSV_matrix(std::string csvf, char c);
