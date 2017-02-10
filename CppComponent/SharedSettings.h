#pragma once

namespace CppComponent
{
    public ref class SharedSettings sealed
    {
    public:
		SharedSettings();
		SharedSettings(Platform::String^ folderName);

		Windows::Foundation::IAsyncAction^ ClearAsync();
		Windows::Foundation::IAsyncOperation<Platform::String^>^ GetAsync(Platform::String^ key);
		Platform::String^ Get(Platform::String^ key);

		Windows::Foundation::IAsyncAction^ SetAsync(Platform::String^ key, Platform::String^ value);
		void Set(Platform::String^ key, Platform::String^ value);

		Windows::Foundation::IAsyncOperation<Platform::String^>^ GetDeviceIdAsync();

	private:
		Windows::Foundation::IAsyncOperation<Platform::String^>^ ReadAsync(Windows::Storage::StorageFolder^ storageFolder, Platform::String^ key);
		Windows::Foundation::IAsyncAction^ WriteAsync(Windows::Storage::StorageFolder^ storageFolder, Platform::String^ key, Platform::String^ value);

		Windows::Foundation::IAsyncOperation<Platform::String^>^ ReadFromCacheAsync(Windows::Storage::StorageFolder^ storageFolder, Platform::String^ key);
		Windows::Foundation::IAsyncAction^ WriteToCacheAsync(Windows::Storage::StorageFolder^ storageFolder, Platform::String^ key, Platform::String^ value);
		
		Platform::String^ ReadFromSettings(Platform::String^ key);
		void WriteToSettings(Platform::String^ key, Platform::String^ value);

		Platform::String^ Generate();

		Platform::String^ mFolderName;
    };
}
