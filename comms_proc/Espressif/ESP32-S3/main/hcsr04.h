#pragma once

#define DEFAULT_AVG_ATTEMPTS_DELAY 5
#define DEFAULT_TEMPERATURE 19.307

/* Initialize the sensor. Return an opaque
 * sensor's handle, or `NULL` on error.
 */
void hcsr04Create(void);

/* Return echo duration in microseconds or -1 on failure. */
long hcsr04GetEcho(void);

/* Return distance in millimiters or 'NAN' on failure. */
float hcsr04GetDistance(void);

/* Return distance in millimiters or 'NAN' on failure. */
float hcsr04GetDistanceTemp(float temperature);

/* Perform multiple measurements and return the average
 * distance in millimiters or 'NAN' on failure.
 */
float hcsr04GetDistanceAvg(int attempts_count, int attempts_delay);

/* Perform multiple measurements and return the average
 * distance in millimiters or 'NAN' on failure.
 */
float hcsr04GetDistanceAvgTemp(int attempts_count, int attempts_delay, float temperature);
