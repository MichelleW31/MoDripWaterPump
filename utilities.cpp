#include "utilities.h"
#include <math.h>

float getMoisturePercentage(int moistureValue){
  // Get Range
  float minValue = 1400;
  float maxValue = 2900;
  float moistureRange = maxValue - minValue;
  float moisturePercentage;

  // Calculate percentage
  if (moistureValue > 2900) {
    moisturePercentage = 0;
  } else if (moistureValue < 1400) {
    moisturePercentage = 100;
  } else {

    moisturePercentage = (int)round(
      ((maxValue - moistureValue) / moistureRange) * 100
    );
  }

  return moisturePercentage;
};