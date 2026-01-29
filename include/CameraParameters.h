#ifndef CAMERAPARAMETERS_H
#define CAMERAPARAMETERS_H

#include <cstdint>

/**
 * @struct CameraParameters
 * @brief Holds all parameters/settings for a single camera
 */
struct CameraParameters
{
	double temperature;	   ///< Current temperature in °C
	double fps;			   ///< Frames per second
	double exposureTime;   ///< Exposure time in µs
	double gain;		   ///< Gain/amplification factor
	bool power_status;	   ///< Power on/off
	uint64_t frame_counter; ///< Total frames captured
	int error_code;		   ///< Error code (0 = no error)

	/**
	 * @brief Default constructor initializing all parameters
	 */
	CameraParameters() :
		temperature( 0.0 ), fps( 0.0 ), exposureTime( 0.0 ), gain( 0.0 ), power_status( false ), frame_counter( 0 ),
		error_code( 0 )
	{
	}
};

#endif // CAMERAPARAMETERS_H
