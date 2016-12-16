#import "NSCoder+MGLAdditions.h"

#import <mbgl/util/feature.hpp>

@interface NSCoder (MGLAdditions_Private)

- (void)mgl_encodeLocationCoordinates2D:(std::vector<CLLocationCoordinate2D>)coordinates forKey:(NSString *)key;

- (std::vector<CLLocationCoordinate2D>)mgl_decodeLocationCoordinates2DForKey:(NSString *)key;

@end

