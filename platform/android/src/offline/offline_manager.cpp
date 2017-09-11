#include "offline_manager.hpp"

#include <mbgl/util/string.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/chrono.hpp>

#include "../attach_env.hpp"
#include "../jni/generic_global_ref_deleter.hpp"

namespace mbgl {
namespace android {

// OfflineManager //

OfflineManager::OfflineManager(jni::JNIEnv& env, jni::Object<FileSource> jFileSource)
    : fileSource(mbgl::android::FileSource::getDefaultFileSource(env, jFileSource)) {
}

OfflineManager::~OfflineManager() {}

void OfflineManager::setOfflineMapboxTileCountLimit(jni::JNIEnv&, jni::jlong limit) {
    fileSource.setOfflineMapboxTileCountLimit(limit);
}

void OfflineManager::listOfflineRegions(jni::JNIEnv& env_, jni::Object<FileSource> jFileSource_, jni::Object<ListOfflineRegionsCallback> callback_) {
    // list regions
    fileSource.listOfflineRegions([
        //Keep a shared ptr to a global reference of the callback and file source so they are not GC'd in the meanwhile
        callback = std::shared_ptr<jni::jobject>(callback_.NewGlobalRef(env_).release()->Get(), GenericGlobalRefDeleter()),
        jFileSource = std::shared_ptr<jni::jobject>(jFileSource_.NewGlobalRef(env_).release()->Get(), GenericGlobalRefDeleter())
    ](std::exception_ptr error, mbgl::optional<std::vector<mbgl::OfflineRegion>> regions) mutable {

        // Reattach, the callback comes from a different thread
        android::UniqueEnv env = android::AttachEnv();

        if (error) {
            OfflineManager::ListOfflineRegionsCallback::onError(*env, jni::Object<ListOfflineRegionsCallback>(*callback), error);
        } else if (regions) {
            OfflineManager::ListOfflineRegionsCallback::onList(*env, jni::Object<FileSource>(*jFileSource), jni::Object<ListOfflineRegionsCallback>(*callback), std::move(regions));
        }
    });
}
void OfflineManager::putResourceWithUrl(jni::JNIEnv& env_, jni::String url_, jni::Array<jni::jbyte> arr){
    auto url =  jni::Make<std::string>(env_, url_);
    auto resource = mbgl::Resource(mbgl::Resource::Kind::Unknown, url);
    mbgl::Response response = mbgl::Response();
    response.expires = mbgl::util::now() + mbgl::Seconds(60 * 60 * 24 * 365);
 
    auto data = std::make_shared<std::string>(arr.Length(env_), char());
    jni::GetArrayRegion(env_, *arr, 0, data->size(), reinterpret_cast<jbyte*>(&(*data)[0]));
    response.data = data;
    
    fileSource.startPut(resource, response, {});
    //delete response;

 } 

 void OfflineManager::clear(jni::JNIEnv&) {
    fileSource.clear();
 }
 
 
 void OfflineManager::putTileWithUrlTemplate(jni::JNIEnv& env_, jni::String urlTemplate_, jfloat pixelRatio, jint x, jint y, jint z, jni::Array<jni::jbyte> arr) {
     auto urlTemplate = jni::Make<std::string>(env_, urlTemplate_);
     mbgl::Resource resource = mbgl::Resource::tile(urlTemplate, pixelRatio, x, y, z, mbgl::Tileset::Scheme::XYZ);
     mbgl::Response response = mbgl::Response();
     response.expires = mbgl::util::now() + mbgl::Seconds(60 * 60 * 24 * 365);
    
     auto length = jni::GetArrayLength(env_, *arr);
     auto data = std::make_shared<std::string>(length, char());
     //auto data = std::make_unique<std::string>(jni::GetArrayLength(env_, *arr), char());
     jni::GetArrayRegion(env_, *arr, 0, data->size(), reinterpret_cast<jbyte*>(&(*data)[0]));
     response.data = data;
    // free(data);
     fileSource.startPut(resource, response, {});
     //Log::Debug(Event::Database, "Size: " + util::toString(sizeof(env_)));

     data.reset();
     response.data = data;
     //Delete references
     //jni::DeleteLocalRef(env_, urlTemplate);
     //jni::DeleteLocalRef(env_, arr);
 }

void OfflineManager::createOfflineRegion(jni::JNIEnv& env_,
                                         jni::Object<FileSource> jFileSource_,
                                         jni::Object<OfflineRegionDefinition> definition_,
                                         jni::Array<jni::jbyte> metadata_,
                                         jni::Object<CreateOfflineRegionCallback> callback_) {
    // Convert

    // XXX hardcoded cast for now as we only support OfflineTilePyramidRegionDefinition
    auto definition = OfflineTilePyramidRegionDefinition::getDefinition(env_, jni::Object<OfflineTilePyramidRegionDefinition>(*definition_));

    mbgl::OfflineRegionMetadata metadata;
    if (metadata_) {
        metadata = OfflineRegion::metadata(env_, metadata_);
    }

    // Create region
    fileSource.createOfflineRegion(definition, metadata, [
        //Keep a shared ptr to a global reference of the callback and file source so they are not GC'd in the meanwhile
        callback = std::shared_ptr<jni::jobject>(callback_.NewGlobalRef(env_).release()->Get(), GenericGlobalRefDeleter()),
        jFileSource = std::shared_ptr<jni::jobject>(jFileSource_.NewGlobalRef(env_).release()->Get(), GenericGlobalRefDeleter())
    ](std::exception_ptr error, mbgl::optional<mbgl::OfflineRegion> region) mutable {

        // Reattach, the callback comes from a different thread
        android::UniqueEnv env = android::AttachEnv();

        if (error) {
            OfflineManager::CreateOfflineRegionCallback::onError(*env, jni::Object<CreateOfflineRegionCallback>(*callback), error);
        } else if (region) {
            OfflineManager::CreateOfflineRegionCallback::onCreate(
                *env,
                jni::Object<FileSource>(*jFileSource),
                jni::Object<CreateOfflineRegionCallback>(*callback), std::move(region)
            );
        }
    });
}

jni::Class<OfflineManager> OfflineManager::javaClass;

void OfflineManager::registerNative(jni::JNIEnv& env) {
    OfflineManager::ListOfflineRegionsCallback::registerNative(env);
    OfflineManager::CreateOfflineRegionCallback::registerNative(env);
    //OfflineManager::PutOfflineCallback::registerNative(env);

    javaClass = *jni::Class<OfflineManager>::Find(env).NewGlobalRef(env).release();

    #define METHOD(MethodPtr, name) jni::MakeNativePeerMethod<decltype(MethodPtr), (MethodPtr)>(name)

    jni::RegisterNativePeer<OfflineManager>( env, javaClass, "nativePtr",
        std::make_unique<OfflineManager, JNIEnv&, jni::Object<FileSource>>,
        "initialize",
        "finalize",
        METHOD(&OfflineManager::setOfflineMapboxTileCountLimit, "setOfflineMapboxTileCountLimit"),
        METHOD(&OfflineManager::listOfflineRegions, "listOfflineRegions"),
	    METHOD(&OfflineManager::createOfflineRegion, "createOfflineRegion"),
        METHOD(&OfflineManager::clear, "clear"),
        METHOD(&OfflineManager::putResourceWithUrl, "putResourceWithUrl"),
        METHOD(&OfflineManager::putTileWithUrlTemplate, "putTileWithUrlTemplate"));}

// OfflineManager::ListOfflineRegionsCallback //

void OfflineManager::ListOfflineRegionsCallback::onError(jni::JNIEnv& env,
                                                          jni::Object<OfflineManager::ListOfflineRegionsCallback> callback,
                                                          std::exception_ptr error) {
    static auto method = javaClass.GetMethod<void (jni::String)>(env, "onError");
    std::string message = mbgl::util::toString(error);
    callback.Call(env, method, jni::Make<jni::String>(env, message));
}

void OfflineManager::ListOfflineRegionsCallback::onList(jni::JNIEnv& env,
                                                        jni::Object<FileSource> jFileSource,
                                                        jni::Object<OfflineManager::ListOfflineRegionsCallback> callback,
                                                        mbgl::optional<std::vector<mbgl::OfflineRegion>> regions) {
    //Convert the regions to java peer objects
    std::size_t index = 0;
    auto jregions = jni::Array<jni::Object<OfflineRegion>>::New(env, regions->size(), OfflineRegion::javaClass);
    for (auto& region : *regions) {
        auto jregion = OfflineRegion::New(env, jFileSource, std::move(region));
        jregions.Set(env, index, jregion);
        jni::DeleteLocalRef(env, jregion);
        index++;
    }

    // Trigger callback
    static auto method = javaClass.GetMethod<void (jni::Array<jni::Object<OfflineRegion>>)>(env, "onList");
    callback.Call(env, method, jregions);
    jni::DeleteLocalRef(env, jregions);
}

jni::Class<OfflineManager::ListOfflineRegionsCallback> OfflineManager::ListOfflineRegionsCallback::javaClass;

void OfflineManager::ListOfflineRegionsCallback::registerNative(jni::JNIEnv& env) {
    javaClass = *jni::Class<OfflineManager::ListOfflineRegionsCallback>::Find(env).NewGlobalRef(env).release();
}

// OfflineManager::CreateOfflineRegionCallback //

void OfflineManager::CreateOfflineRegionCallback::onError(jni::JNIEnv& env,
                                                          jni::Object<OfflineManager::CreateOfflineRegionCallback> callback,
                                                          std::exception_ptr error) {
    static auto method = javaClass.GetMethod<void (jni::String)>(env, "onError");
    std::string message = mbgl::util::toString(error);
    callback.Call(env, method, jni::Make<jni::String>(env, message));
}

void OfflineManager::CreateOfflineRegionCallback::onCreate(jni::JNIEnv& env,
                                                        jni::Object<FileSource> jFileSource,
                                                        jni::Object<OfflineManager::CreateOfflineRegionCallback> callback,
                                                        mbgl::optional<mbgl::OfflineRegion> region) {
    //Convert the region to java peer object
    auto jregion = OfflineRegion::New(env, jFileSource, std::move(*region));

    // Trigger callback
    static auto method = javaClass.GetMethod<void (jni::Object<OfflineRegion>)>(env, "onCreate");
    callback.Call(env, method, jregion);
    jni::DeleteLocalRef(env, jregion);
}

jni::Class<OfflineManager::CreateOfflineRegionCallback> OfflineManager::CreateOfflineRegionCallback::javaClass;

void OfflineManager::CreateOfflineRegionCallback::registerNative(jni::JNIEnv& env) {
    javaClass = *jni::Class<OfflineManager::CreateOfflineRegionCallback>::Find(env).NewGlobalRef(env).release();
}

} // namespace android
} // namespace mbgl
