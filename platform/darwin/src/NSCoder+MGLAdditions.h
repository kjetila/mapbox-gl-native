#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@interface NSCoder (MGLAdditions)

- (void)encodeMGLCoordinate:(CLLocationCoordinate2D)coordinate forKey:(NSString *)key;

- (CLLocationCoordinate2D)decodeMGLCoordinateForKey:(NSString *)key;

@end
