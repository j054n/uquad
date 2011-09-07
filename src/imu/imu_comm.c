#include "imu_comm.h"

static double grad2rad(double degrees){
    // PI/180 == 0.017453292519943295
    return degrees*0.017453292519943295;
}

static double rad2grad(double radians){
    // 180/PI == 57.29577951308232
    return radians*57.29577951308232;
}

static int gyro_scale_adjust(struct imu * imu, double * gyro_reading){
    //TODO Implement scale calibration,
    // Note: Should be /300, but /450 seems to work better.
    // Will be sensor specific
    // Should get a true calibration instead of this.
    *gyro_reading *= 0.92955;
    return ERROR_OK;
}

static int acc_scale_adjust(struct imu * imu, double * acc_reading){
    //TODO Implement scale calibration,
    // Will be sensor specific
    // Should get a true calibration instead of this.
    return ERROR_OK;
}

static double imu_sens_g = {1.5,2,4,6};
static int imu_sens_mv_per_g = {800,600,400,300};
int imu_get_sens(int sens){
    assert(sens<IMU_SENS_OPT_COUNT);
    return imu_sens_mv_per_g[sens];
}   

/** 
 * Reads data until a frame init char is found.
 * Keeps going until a end char is found. Then stops.
 * 
 * @param imu 
 * @param frame New frame is returned here.
 * 
 * @return error code
 */
static int imu_read_frame(struct imu * imu){
    int retval = ERROR_OK,watchdog,read;
    unsigned char tmp = '@';// Anything diff from IMU_FRAME_INIT_CHAR
    struct imu_frame * new_frame;
    new_frame = imu->frame_buffer[imu->frames_sampled];
    watchdog = 0;
    while(watchdog < IMU_FRAME_SIZE_BYTES_DEFAULT){
	retval = fread(&tmp,IMU_INIT_END_SIZE,1,imu->device);
	if(retval == 0){
	    perror("Read error...");
	    return ERROR_READ_TIMEOUT;
	}else{
	    if(tmp == IMU_FRAME_INIT_CHAR)
		break;
	}
	++watchdog;
    }
    if(watchdog>=IMU_FRAME_SIZE_BYTES_DEFAULT)
	return ERROR_READ_SYNC;

    // At this point we should be ready to read the frame
    // init char already read

    // Get count

    watchdog = 0;
    while(watchdog < READ_RETRIES){
	retval = fread(&tmp,IMU_INIT_END_SIZE,1,imu->device);
	if(retval == 0){
	    perror("Read error...");
	    return ERROR_READ_TIMEOUT;
	}else{
	    new_frame->count = (unsigned int) tmp;
	    break;
	}
	++watchdog;
    }
    if(watchdog>=READ_RETRIES)
	return ERROR_READ_TIMEOUT;
  
    // Generate timestamp

    gettimeofday(new_frame->timestamp,NULL);

    // Read sensors RAW data

    watchdog = 0;
    read = 0;
    while(watchdog < READ_RETRIES){
	retval = fread(new_frame->raw + read,IMU_BYTES_PER_SENSOR,IMU_SENSOR_COUNT,imu->device);
	if(retval == 0){
	    perror("Read error...");
	    return ERROR_READ_TIMEOUT;
	}else{
	    read += retval;
	    if(read == IMU_SENSOR_COUNT)
		// done reading
		break;
	}
	++watchdog;
    }
    if(watchdog>=READ_RETRIES)
	return ERROR_READ_TIMEOUT;

    // Now get the end char

    watchdog = 0;
    while(watchdog < READ_RETRIES){
	retval = fread(&tmp,IMU_INIT_END_SIZE,1,imu->device);
	if(retval == 0){
	    perror("Read error...");
	    return ERROR_READ_TIMEOUT;
	}else{
	    if(tmp == IMU_FRAME_END_CHAR)
		break;
	    else
		return ERROR_READ_SYNC;
	}
	++watchdog;
    }
    if(watchdog>=READ_RETRIES)
	return ERROR_READ_TIMEOUT;


    // Everything went ok

    imu->frames_sampled += 1;
    return ERROR_OK;
}

/**
 * Convert data read from gyro to rad/s
 * 
 * Gyro outputs ~1.65v for 0deg/sec, then this goes through a
 * 10bit ADC on the Atmega which compares 0-3.3v.
 * The data received is the result of the ADC.
 * 
 * @param imu 
 * @param frame Contains RAW data from IMU
 * @param gyro_reading Rate in rad/sec
 * 
 * @return error code
 */
static int imu_gyro_read(struct imu * imu, struct frame * frame, double * gyro_reading){
    int retval = ERROR_OK, i;
    for(i = 0; i<IMU_GYROS; ++i){
	gyro_reading[i] = ((double) frame->raw[IMU_ACCS + i]) - imu->null_estimates->xyzrpy[IMU_ACCS + i];
	gyro_reading[i] = grad2rad(gyro_reading[i]);
	retval = gyro_scale_adjust(imu,gyro_reading+i);
	err_propagate(retval);
    }
    return retval;
}

/** 
 * 
 * 
 * @param imu 
 * @param acc_data Raw data from IMU
 * @param acc_reading Acceleration, in m/s^2
 * 
 * @return error code
 */
static int imu_acc_read(struct imu * imu, struct frame * frame, double * acc_reading){
    int retval = ERROR_OK, i;
    for(i = 0; i<IMU_ACCS; ++i){
	acc_reading[i] = ((double) frame->raw[i]) - imu->null_estimates->xyzrpy[i];
	acc_reading[i] = grad2rad(acc_reading[i]);
	retval = acc_scale_adjust(imu,acc_reading+i);
	err_propagate(retval);
    }
    return retval;
}

/** 
 * Calculates value of the sensor reading from the RAW data, using current imu calibration.
 * 
 * @param imu Current imu status
 * @param xyzrpy Answer is returned here
 * 
 * @return error code
 */
static int imu_calc_sensors(struct imu * imu, double * xyzrpy){
    int retval = ERROR_OK, iter;

    // Get ACC readings

    for(iter = 0; iter < IMU_ACCS; ++iter){
	retval = imu_acc_read(xyzrpy[iter], frame->raw[iter]);
	err_propagate(retval);
    }
	
    // Get gyro reading

    for(;iter<IMU_SENSOR_COUNT;+iter){
	retval = imu_gyro_read(xyzrpy[iter], frame->raw[iter]);
	err_propagate(retval);
    }
    return retval;
}