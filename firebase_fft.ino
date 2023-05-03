/*
Sets of code belonging to certain aspect of the whole,
- Firebase Set - dealing with connection with Google Firebase DB
- ADXL345 Set - dealing with interactions with ADXL345 3-axis accelerometer
- FFT Set - Dealing with calculations related to FFT algorithm

Program Flow: -
1> Functional Declarations for each set  - prerequisites
2> Setup and Loop and required variables - the actual show
3> Function definitions                  - the backstage setting
*/

// FUNCTIONAL DECLARATIONS
// ______________________________________________________________________________________
// Declarations of the Firebase Set
#include "required.h" // required for Firebase constants

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;          // Variable to save USER UID

String databasePath; // Database main path (to be updated in setup with the user UID)
// Database child nodes
String X_accel = "/X_accel";
String Y_accel = "/Y_accel";
String Z_accel = "/Z_accel";
String timePath = "/timestamp";
String freq = "/Fundamental_Frequency";
String location = "/loc_latlong";

String parentPath;   // Parent Node (to be updated in every loop)

FirebaseJson json;

WiFiUDP ntpUDP;      // Define NTP Client to get time
NTPClient timeClient(ntpUDP, "pool.ntp.org");
int timestamp;       // Variable to save current epoch time

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;     // Timer variables (send new readings every 10 sec) 
unsigned int init_delay = 5000;

// ......................................................................................

bool initWiFi(void);
unsigned long getTime(void);
bool firebase_init(void);

// ______________________________________________________________________________________
// Declarations of the ADXL345 Set

#include <Wire.h>               // I2C communication library
#include <Adafruit_Sensor.h>    // Adafruit  sensor library
#include <Adafruit_ADXL345_U.h> // ADXL345 library
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(); /* ADXL345 Object */
sensors_event_t event;

// ......................................................................................

bool initADXL(void);
// bool get_accel_data(void);

// ______________________________________________________________________________________
// Declarations of the FFT Set

#include "FFT.h" // include the library
//#include "FFT_signal.h"

// signal parameters

#define FFT_N 16        // Sample size must be a power of 2
#define TOTAL_TIME 0.1  // = FFT_N / sampling_freq = 1024 / 200 ; The time in which data was captured.

// #define num_batch 5

float fft_input[FFT_N];
float fft_output[FFT_N];
double fft_signal[FFT_N];

float max_magnitude = 0;
float fundamental_freq = 0;

bool do_fft(void);

// ______________________________________________________________________________________

bool check_init_ADXL, check_init_WiFi, check_init_Firebase, check_fetchGPS; // variables for checking the work of function

bool fetchGPS()
{
  check_fetchGPS = false;
  WiFiClient client;
  HTTPClient http;

  String serverPath = serverName;// + "?temperature=24.37";
  
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverPath.c_str());

  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    if(httpResponseCode == 200)
    {
      Location = http.getString();
      Serial.println(Location);
      http.end(); // Free up resources
      return true;
    } 
    else { http.end(); return false; }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    http.end(); // Free up resources
    return false;
  }
  // Free resources
  

}

void setup()
{  
  Serial.begin(115200); // begin Serial comm

  check_init_ADXL = false;
  check_init_ADXL = initADXL();  // Initialize ADXL sensor
  if(check_init_ADXL == true)    // returning to current function to restore continuity and not upset WDT
  { Serial.printf("\n - - - - - - - - - - ADXL345 initialized successfully !! - - - - - - - - - - \n"); }
  while((millis() - init_delay) < 0) { ; }
  
  check_init_WiFi = false;
  check_init_WiFi = initWiFi();  // Get internet connection
  if(check_init_WiFi == true)    // returning to current function to restore continuity and not upset WDT
  { Serial.printf("\n - - - - - - - - - - WiFi initialized successfully !! - - - - - - - - - - \n"); }
  while((millis() - init_delay) < init_delay) { ; }
  
  timeClient.begin();
  Serial.printf("\n- - - - - Time client begun !! - - - - - \n");
  while((millis() - init_delay) < 0) { ; }
  
  check_init_Firebase = false;
  check_init_Firebase = firebase_init(); // run the required steps for establishing connection with Firebase
  if(check_init_Firebase == true)        // returning to current function to restore continuity and not upset WDT
  { Serial.printf("\n - - - - - - - - - - FIREBASE initialized successfully !! - - - - - - - - - - \n"); }
  while((millis() - init_delay) < 0) { ; }

}

static long data_count = 0;


// ______________________________________________________________________________________

volatile float dc, funda_freq, mag;      // variables for storing FFT output
volatile float sample_time, fft_time;    // variables for storing required time 
static int loop_num = 0;                 // variable for number of samples recorded till date
volatile bool check_fft, check_get_data; // variables for checking the work of function
volatile long int t1, t2;                // variables for time keeping
volatile int i, j, q, k;                 // variables for the running loops

void loop()
{
  Serial.printf("\nLoop Has begun !\n");
  while((millis() - init_delay) < 0) { ; }

  // check_get_data = false;
  // t1 = micros();
  // check_get_data = get_accel_data();
  // if(check_get_data == true)
  // { t2 = micros(); sample_time = ((t2-t1)*1.0/1000); }

  check_fft = false;  
  check_fft = do_fft();
  if(check_fft == true) { Serial.printf("\n - - - - - FFT performed on sample %d - - - - -\n",(loop_num+1)); }
  // while((millis() - init_delay) < 0) { ; }

  // conveying time required
  Serial.printf("\nTime for Data collection = %lf ms\nTime for FFT calculation = %lf ms\n",fft_time);
  String time = String(sample_time) + " - " + String(fft_time) + " - " + String(data_count);
  
  // Send new readings to database
  if (Firebase.ready() && (check_fft) /*((millis() - sendDataPrevMillis > timerDelay) || sendDataPrevMillis == 0)*/)
  {
    // sendDataPrevMillis = millis();
    data_count++;
    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (time);
    
    String(timestamp) = String(timestamp) + " - " + String(data_count);
    parentPath= databasePath + "/" + String(timestamp);

    json.set(X_accel.c_str(), String(event.acceleration.x));
    Serial.printf("\nX-accel = %f\n",event.acceleration.x);
    
    json.set(Y_accel.c_str(), String(event.acceleration.y));
    Serial.printf("Y-accel = %f\n",event.acceleration.y);
    
    json.set(Z_accel.c_str(), String(fft_signal[(FFT_N - 1)]));
    Serial.printf("Z-accel = %f\n",fft_signal[(FFT_N - 1)]);
    
    json.set(freq.c_str(), String(funda_freq)); 
    Serial.printf("Fundamental frequency = %lf\n",funda_freq);
      
    json.set(timePath, String(time));
    check_fetchGPS = fetchGPS();
    if(check_fetchGPS == true) { json.set(location.c_str(), Location); }
    //Serial.printf("Fundamental frequency = %d\n",timestamp);
    
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }  

  loop_num++;
  delay(1000);
}

// FUNCTION DEFINITIONS
// ______________________________________________________________________________________
// Definition of Functions from Firebase set

bool initWiFi(void)         // Initialize WiFi 
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("\nConnecting to WiFi ..\n");
  while (WiFi.status() != WL_CONNECTED) { Serial.printf(".. connecting ..\n"); delay(1000); }
  Serial.printf("\n- - - - - - - - - - Internet Connection established !! - - - - - - - - - -\nIP-Address = ");
  Serial.println(WiFi.localIP());
  return true;
}

unsigned long getTime(void) // Function that gets current epoch time
{
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

bool firebase_init(void)    // Firebase initialization
{
  
  
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = max_retry;
  

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  Serial.printf("\ninside Firebase init func\n");
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID\n");
  while ((auth.token.uid) == "")\
  { Serial.printf(".. getting uid ..\n"); delay(1000); }

  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: "); Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

  return true;
}

// ______________________________________________________________________________________
// Definition of Functions from ADXL345 set

bool initADXL(void)       // Initialize ADXL345
{
  while (!accel.begin())   
  /* If not found, repeat the loop; otherwise, print FOUND. */
  { 
    Serial.println("\n - - - - - - - - - - ADXL345 not found - - - - - - - - - - \n"); 
    delay(500); 
  }
  Serial.println("\n - - - - - - - - - - ADXL345 FOUND !! - - - - - - - - - - \n");  
  return true;
}

// bool get_accel_data(void) // Read accelerometer data from sensor into fft_signal
// {
//   /* Collecting data from accelerometer */
//   int j;
//   for(j = 0; j < FFT_N; j++)
//   { 
//     accel.getEvent(&event);
//     fft_signal[j] = event.acceleration.z;
//   }    
//   // Serial.printf("\nX = %lf, Y = %lf, Z = %lf ,\nSample %d recorded\n",fft_signal[0][j],fft_signal[1][j],fft_signal[2][j],j);
//   // delay(10);
//   // Serial.printf("\n - - - - - Sample Set %d Recorded !!- - - - - \n",(loop_num+1)); 
//   return true;
// }

// ______________________________________________________________________________________
// Definition of Functions from FFT set

bool do_fft()
{
  t1 = micros();
  fft_config_t *real_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, fft_input, fft_output);
  int q;
  float mag,freq;
  
  /* Putting the real-time value in the input buffer */
  for (q = 0 ; q < FFT_N ; q++) { accel.getEvent(&event); real_fft_plan->input[q] = event.acceleration.z; }
  Serial.printf("\n - - - - - Sample Set %d Recorded !!- - - - - \n",(loop_num+1));
  
  fft_execute(real_fft_plan);                     // Execute transformation

  for (k = 1 ; k < real_fft_plan->size / 2 ; k++) // calculate the output
  {
    /*The real part of a magnitude at a frequency is followed by the corresponding imaginary part in the output*/
    mag = sqrt(pow(real_fft_plan->output[2*k],2) + pow(real_fft_plan->output[2*k+1],2))/1.0;
    mag = (mag*2/FFT_N);
    freq = k*1.0/TOTAL_TIME;
    Serial.printf("\nFrequency = %f Hz ; Mag = %f\n",freq,mag);
    // if(mag > max_magnitude)
    // {
    //   max_magnitude = mag;
    //   funda_freq = freq;
    // }
  }
  t2 = micros();

  while((millis() - init_delay) < 0) { ; }  
  

  /*Multiply the magnitude of the DC component with (1/FFT_N) to obtain the DC component*/
  dc = ((real_fft_plan->output[0])/FFT_N);
  Serial.printf("\nFrequency = 0 Hz ; DC Mag = %f\n",dc);

  /*Multiply the magnitude at all other frequencies with (2/FFT_N) to obtain the amplitude at that frequency*/
  fft_time = ((t2-t1)*1.0/1000);
  
  fft_destroy(real_fft_plan); // Clean up at the end to free the memory allocated

  return true;
}

