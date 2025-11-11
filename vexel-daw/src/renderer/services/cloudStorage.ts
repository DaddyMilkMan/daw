/**
 * Cloud Storage Service
 * Integration with S3, Google Drive, and local IndexedDB caching
 *
 * Features:
 * - Upload/download projects to cloud
 * - IndexedDB caching for offline access
 * - Chunked multipart uploads for large audio files
 * - Auto-save to cloud
 * - Cloud project browser
 * - Sync status tracking
 *
 * Supported Providers:
 * - AWS S3
 * - Google Drive API
 * - Local IndexedDB (fallback)
 *
 * Based on:
 * - AWS SDK for JavaScript v3
 * - Google Drive API v3
 * - IndexedDB API
 */

import { openDB, DBSchema, IDBPDatabase } from 'idb';
import { S3Client, GetObjectCommand, ListObjectsV2Command, PutObjectCommand, DeleteObjectCommand } from '@aws-sdk/client-s3';
import { Upload } from '@aws-sdk/lib-storage';

export type CloudProvider = 's3' | 'google-drive' | 'local';

export interface CloudStorageConfig {
  provider: CloudProvider;
  credentials?: {
    // S3
    accessKeyId?: string;
    secretAccessKey?: string;
    region?: string;
    bucket?: string;

    // Google Drive
    clientId?: string;
    apiKey?: string;
    accessToken?: string;
  };
}

export interface CloudProject {
  id: string;
  name: string;
  path: string;
  size: number;
  lastModified: number;
  provider: CloudProvider;
  syncStatus: 'synced' | 'uploading' | 'downloading' | 'conflict' | 'error';
  localCached: boolean;
}

export interface UploadProgress {
  projectId: string;
  fileName: string;
  loaded: number;
  total: number;
  percentage: number;
  status: 'uploading' | 'completed' | 'error';
}

interface ZenithDB extends DBSchema {
  projects: {
    key: string;
    value: {
      id: string;
      name: string;
      data: Blob;
      lastModified: number;
      size: number;
    };
  };
  audioFiles: {
    key: string;
    value: {
      id: string;
      name: string;
      blob: Blob;
      lastModified: number;
      size: number;
    };
  };
  cache: {
    key: string;
    value: {
      key: string;
      data: any;
      timestamp: number;
    };
  };
}

export class CloudStorageService {
  private static instance: CloudStorageService | null = null;
  private config: CloudStorageConfig | null = null;
  private db: IDBPDatabase<ZenithDB> | null = null;
  private uploadListeners: ((progress: UploadProgress) => void)[] = [];
  private autoSaveInterval: NodeJS.Timeout | null = null;
  private lastSaveTime: number = 0;
  private autoSaveCallback: (() => Promise<void>) | null = null;

  private constructor() {}

  static getInstance(): CloudStorageService {
    if (!CloudStorageService.instance) {
      CloudStorageService.instance = new CloudStorageService();
    }
    return CloudStorageService.instance;
  }

  /**
   * Initialize cloud storage
   */
  async initialize(config: CloudStorageConfig): Promise<void> {
    console.log(`Initializing cloud storage: ${config.provider}`);

    this.config = config;

    // Initialize IndexedDB
    this.db = await openDB<ZenithDB>('zenith-daw-storage', 1, {
      upgrade(db) {
        // Projects store
        if (!db.objectStoreNames.contains('projects')) {
          db.createObjectStore('projects', { keyPath: 'id' });
        }

        // Audio files store
        if (!db.objectStoreNames.contains('audioFiles')) {
          db.createObjectStore('audioFiles', { keyPath: 'id' });
        }

        // Cache store
        if (!db.objectStoreNames.contains('cache')) {
          db.createObjectStore('cache', { keyPath: 'key' });
        }
      },
    });

    console.log('Cloud storage initialized');
  }

  /**
   * Upload project to cloud
   */
  async uploadProject(
    projectName: string,
    projectData: Blob,
    audioFiles: { name: string; blob: Blob }[]
  ): Promise<CloudProject> {
    if (!this.config) {
      throw new Error('Cloud storage not initialized');
    }

    console.log(`Uploading project: ${projectName} (${projectData.size} bytes)`);

    const projectId = `project-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    // Cache locally first
    await this.cacheProjectLocally(projectId, projectName, projectData);

    for (const audioFile of audioFiles) {
      await this.cacheAudioFileLocally(
        `${projectId}-${audioFile.name}`,
        audioFile.name,
        audioFile.blob
      );
    }

    // Upload based on provider
    switch (this.config.provider) {
      case 's3':
        await this.uploadToS3(projectId, projectName, projectData, audioFiles);
        break;

      case 'google-drive':
        await this.uploadToGoogleDrive(projectId, projectName, projectData, audioFiles);
        break;

      case 'local':
        // Already cached, nothing more to do
        break;
    }

    const project: CloudProject = {
      id: projectId,
      name: projectName,
      path: `/${projectName}`,
      size: projectData.size,
      lastModified: Date.now(),
      provider: this.config.provider,
      syncStatus: 'synced',
      localCached: true,
    };

    console.log(`Project uploaded: ${projectId}`);

    return project;
  }

  /**
   * Download project from cloud (with version sync detection)
   */
  async downloadProject(
    projectId: string,
    forceDownload: boolean = false
  ): Promise<{ data: Blob; audioFiles: { name: string; blob: Blob }[]; isNewerVersion: boolean }> {
    if (!this.config) {
      throw new Error('Cloud storage not initialized');
    }

    console.log(`Downloading project: ${projectId}`);

    // Check local cache first
    const cached = await this.getCachedProject(projectId);

    // Get cloud version info
    let cloudLastModified = 0;
    const projects = await this.listProjects();
    const cloudProject = projects.find(p => p.id === projectId);
    if (cloudProject) {
      cloudLastModified = cloudProject.lastModified;
    }

    // Check if cloud version is newer
    const isNewerVersion = cached && cloudLastModified > cached.lastModified;

    if (cached && !forceDownload && !isNewerVersion) {
      console.log('✅ Using cached version (up to date)');
      return {
        data: cached.data,
        audioFiles: [], // TODO: Retrieve cached audio files
        isNewerVersion: false,
      };
    }

    if (isNewerVersion) {
      console.log('⚠️ Newer version available in cloud - downloading...');
    }

    // Download from provider
    let result: { data: Blob; audioFiles: { name: string; blob: Blob }[] };

    switch (this.config.provider) {
      case 's3':
        result = await this.downloadFromS3(projectId);
        break;

      case 'google-drive':
        result = await this.downloadFromGoogleDrive(projectId);
        break;

      case 'local':
        throw new Error('Project not found in local storage');

      default:
        throw new Error('Invalid cloud provider');
    }

    // Update cache with new version
    await this.cacheProjectLocally(
      projectId,
      cloudProject?.name || projectId,
      result.data
    );

    return {
      ...result,
      isNewerVersion: isNewerVersion || false,
    };
  }

  /**
   * Check if cloud has newer version than local
   */
  async hasNewerVersion(projectId: string): Promise<boolean> {
    const cached = await this.getCachedProject(projectId);
    if (!cached) return true; // No local version

    const projects = await this.listProjects();
    const cloudProject = projects.find(p => p.id === projectId);

    if (!cloudProject) return false; // No cloud version

    return cloudProject.lastModified > cached.lastModified;
  }

  /**
   * List projects in cloud
   */
  async listProjects(): Promise<CloudProject[]> {
    if (!this.config) {
      throw new Error('Cloud storage not initialized');
    }

    console.log('Listing cloud projects');

    // Get local cached projects
    const localProjects = await this.listLocalProjects();

    // Get cloud projects based on provider
    let cloudProjects: CloudProject[] = [];

    switch (this.config.provider) {
      case 's3':
        cloudProjects = await this.listS3Projects();
        break;

      case 'google-drive':
        cloudProjects = await this.listGoogleDriveProjects();
        break;

      case 'local':
        cloudProjects = localProjects;
        break;
    }

    // Merge local and cloud projects
    const merged = this.mergeProjectLists(localProjects, cloudProjects);

    console.log(`Found ${merged.length} projects`);

    return merged;
  }

  /**
   * Delete project from cloud
   */
  async deleteProject(projectId: string): Promise<void> {
    if (!this.config) {
      throw new Error('Cloud storage not initialized');
    }

    console.log(`Deleting project: ${projectId}`);

    // Delete from cache
    await this.deleteCachedProject(projectId);

    // Delete from cloud
    switch (this.config.provider) {
      case 's3':
        await this.deleteFromS3(projectId);
        break;

      case 'google-drive':
        await this.deleteFromGoogleDrive(projectId);
        break;

      case 'local':
        // Already deleted from cache
        break;
    }

    console.log(`Project deleted: ${projectId}`);
  }

  /**
   * Enable auto-save with throttling
   * @param intervalMs - Throttle interval (recommended: 3000-5000ms for UX)
   * @param saveCallback - Function that returns project data to save
   */
  enableAutoSave(
    intervalMs: number = 5000,
    saveCallback: () => Promise<{ projectId: string; projectName: string; projectData: Blob; audioFiles: { name: string; blob: Blob }[] }>
  ): void {
    if (this.autoSaveInterval) {
      clearInterval(this.autoSaveInterval);
    }

    // Throttled auto-save: saves at regular intervals
    // Uses throttle (not debounce) to ensure regular saves even during continuous editing
    this.autoSaveInterval = setInterval(async () => {
      const now = Date.now();
      const timeSinceLastSave = now - this.lastSaveTime;

      // Skip if saved too recently (throttle)
      if (timeSinceLastSave < intervalMs) {
        return;
      }

      try {
        console.log('💾 Auto-saving project...');
        const data = await saveCallback();

        // Upload to cloud
        await this.uploadProject(data.projectName, data.projectData, data.audioFiles);

        this.lastSaveTime = now;
        console.log('✅ Auto-save completed');
      } catch (error) {
        console.error('❌ Auto-save failed:', error);
        // Don't throw - auto-save should be non-blocking
      }
    }, intervalMs);

    console.log(`✅ Auto-save enabled (throttle interval: ${intervalMs}ms)`);
  }

  /**
   * Disable auto-save
   */
  disableAutoSave(): void {
    if (this.autoSaveInterval) {
      clearInterval(this.autoSaveInterval);
      this.autoSaveInterval = null;
      this.autoSaveCallback = null;
    }

    console.log('Auto-save disabled');
  }

  /**
   * Manually trigger a save (bypasses throttle)
   */
  async forceSave(
    projectId: string,
    projectName: string,
    projectData: Blob,
    audioFiles: { name: string; blob: Blob }[]
  ): Promise<void> {
    console.log('💾 Force saving project...');
    await this.uploadProject(projectName, projectData, audioFiles);
    this.lastSaveTime = Date.now();
    console.log('✅ Force save completed');
  }

  /**
   * Cache project locally in IndexedDB
   */
  private async cacheProjectLocally(
    projectId: string,
    projectName: string,
    projectData: Blob
  ): Promise<void> {
    if (!this.db) return;

    await this.db.put('projects', {
      id: projectId,
      name: projectName,
      data: projectData,
      lastModified: Date.now(),
      size: projectData.size,
    });

    console.log(`Project cached locally: ${projectId}`);
  }

  /**
   * Cache audio file locally
   */
  private async cacheAudioFileLocally(
    fileId: string,
    fileName: string,
    blob: Blob
  ): Promise<void> {
    if (!this.db) return;

    await this.db.put('audioFiles', {
      id: fileId,
      name: fileName,
      blob,
      lastModified: Date.now(),
      size: blob.size,
    });

    console.log(`Audio file cached locally: ${fileName}`);
  }

  /**
   * Get cached project
   */
  private async getCachedProject(projectId: string): Promise<{ name: string; data: Blob } | null> {
    if (!this.db) return null;

    const project = await this.db.get('projects', projectId);
    if (!project) return null;

    return {
      name: project.name,
      data: project.data,
    };
  }

  /**
   * Delete cached project
   */
  private async deleteCachedProject(projectId: string): Promise<void> {
    if (!this.db) return;

    await this.db.delete('projects', projectId);
    console.log(`Deleted cached project: ${projectId}`);
  }

  /**
   * List local projects
   */
  private async listLocalProjects(): Promise<CloudProject[]> {
    if (!this.db) return [];

    const projects = await this.db.getAll('projects');

    return projects.map((p) => ({
      id: p.id,
      name: p.name,
      path: `/${p.name}`,
      size: p.size,
      lastModified: p.lastModified,
      provider: 'local' as CloudProvider,
      syncStatus: 'synced' as const,
      localCached: true,
    }));
  }

  /**
   * AWS S3 Upload (chunked multipart for large files)
   */
  private async uploadToS3(
    projectId: string,
    projectName: string,
    projectData: Blob,
    audioFiles: { name: string; blob: Blob }[]
  ): Promise<void> {
    console.log('Uploading to S3...');

    if (!this.config?.credentials) {
      throw new Error('S3 credentials not configured');
    }

    const { accessKeyId, secretAccessKey, region, bucket } = this.config.credentials;

    if (!accessKeyId || !secretAccessKey || !region || !bucket) {
      throw new Error('Missing S3 credentials');
    }

    try {
      // Initialize S3 client
      const s3Client = new S3Client({
        region,
        credentials: {
          accessKeyId,
          secretAccessKey,
        },
      });

      const projectKey = `projects/${projectId}.json`;
      const timestamp = Date.now();

      // Convert Blob to ArrayBuffer for upload
      const arrayBuffer = await projectData.arrayBuffer();

      // Use Upload for multipart (handles large files automatically)
      const upload = new Upload({
        client: s3Client,
        params: {
          Bucket: bucket,
          Key: projectKey,
          Body: new Uint8Array(arrayBuffer),
          ContentType: 'application/json',
          Metadata: {
            'project-name': projectName,
            'project-id': projectId,
            'last-modified': timestamp.toString(),
            'version': '1.0',
          },
        },
      });

      // Track upload progress
      upload.on('httpUploadProgress', (progress) => {
        if (progress.loaded && progress.total) {
          this.emitProgress({
            projectId,
            fileName: projectName,
            loaded: progress.loaded,
            total: progress.total,
            percentage: Math.round((progress.loaded / progress.total) * 100),
            status: 'uploading',
          });
        }
      });

      // Perform upload
      const result = await upload.done();
      console.log('✅ S3 upload complete:', result);

      // Upload audio files (if any)
      for (const audioFile of audioFiles) {
        const audioKey = `projects/${projectId}/audio/${audioFile.name}`;
        const audioBuffer = await audioFile.blob.arrayBuffer();

        const audioUpload = new Upload({
          client: s3Client,
          params: {
            Bucket: bucket,
            Key: audioKey,
            Body: new Uint8Array(audioBuffer),
            ContentType: 'audio/wav',
          },
        });

        await audioUpload.done();
        console.log(`✅ Uploaded audio file: ${audioFile.name}`);
      }

      // Emit final completion
      this.emitProgress({
        projectId,
        fileName: projectName,
        loaded: projectData.size,
        total: projectData.size,
        percentage: 100,
        status: 'completed',
      });

      console.log(`✅ Project "${projectName}" uploaded to S3 successfully`);
    } catch (error) {
      console.error('❌ S3 upload failed:', error);
      this.emitProgress({
        projectId,
        fileName: projectName,
        loaded: 0,
        total: projectData.size,
        percentage: 0,
        status: 'error',
      });
      throw new Error(`S3 upload failed: ${error}`);
    }
  }

  /**
   * AWS S3 Download
   */
  private async downloadFromS3(projectId: string): Promise<{ data: Blob; audioFiles: { name: string; blob: Blob }[] }> {
    console.log('Downloading from S3...');

    if (!this.config?.credentials) {
      throw new Error('S3 credentials not configured');
    }

    const { accessKeyId, secretAccessKey, region, bucket } = this.config.credentials;

    if (!accessKeyId || !secretAccessKey || !region || !bucket) {
      throw new Error('Missing S3 credentials');
    }

    try {
      // Initialize S3 client
      const s3Client = new S3Client({
        region,
        credentials: {
          accessKeyId,
          secretAccessKey,
        },
      });

      // Download project file
      const projectKey = `projects/${projectId}.json`;
      console.log(`Downloading project from S3: ${bucket}/${projectKey}`);

      const command = new GetObjectCommand({
        Bucket: bucket,
        Key: projectKey,
      });

      const response = await s3Client.send(command);

      // Convert stream to blob
      const projectData = await this.streamToBlob(response.Body as ReadableStream);

      console.log('✅ Project downloaded from S3');

      // Download associated audio files (if any)
      // This would need to be enhanced based on project structure
      const audioFiles: { name: string; blob: Blob }[] = [];

      return {
        data: projectData,
        audioFiles,
      };
    } catch (error) {
      console.error('❌ S3 download failed:', error);
      throw new Error(`S3 download failed: ${error}`);
    }
  }

  /**
   * Convert ReadableStream to Blob
   */
  private async streamToBlob(stream: ReadableStream | Blob | Uint8Array | any): Promise<Blob> {
    // Handle different response types
    if (stream instanceof Blob) {
      return stream;
    }

    if (stream instanceof Uint8Array) {
      return new Blob([stream]);
    }

    // Handle ReadableStream (browser)
    if (stream && typeof stream.getReader === 'function') {
      const reader = stream.getReader();
      const chunks: Uint8Array[] = [];

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        chunks.push(value);
      }

      return new Blob(chunks);
    }

    // Handle Node.js stream
    if (stream && typeof stream.on === 'function') {
      return new Promise((resolve, reject) => {
        const chunks: Uint8Array[] = [];
        stream.on('data', (chunk: Uint8Array) => chunks.push(chunk));
        stream.on('error', reject);
        stream.on('end', () => resolve(new Blob(chunks)));
      });
    }

    throw new Error('Unsupported stream type');
  }

  /**
   * AWS S3 List Projects
   */
  private async listS3Projects(): Promise<CloudProject[]> {
    console.log('Listing S3 projects...');

    if (!this.config?.credentials) {
      return [];
    }

    const { accessKeyId, secretAccessKey, region, bucket } = this.config.credentials;

    if (!accessKeyId || !secretAccessKey || !region || !bucket) {
      return [];
    }

    try {
      const s3Client = new S3Client({
        region,
        credentials: {
          accessKeyId,
          secretAccessKey,
        },
      });

      const command = new ListObjectsV2Command({
        Bucket: bucket,
        Prefix: 'projects/',
      });

      const response = await s3Client.send(command);

      if (!response.Contents || response.Contents.length === 0) {
        return [];
      }

      const projects: CloudProject[] = await Promise.all(
        response.Contents.filter(item => item.Key?.endsWith('.json')).map(async (item) => {
          const id = item.Key!.replace('projects/', '').replace('.json', '');

          // Get metadata by fetching object head
          try {
            const headCommand = new GetObjectCommand({
              Bucket: bucket,
              Key: item.Key!,
            });
            const headResponse = await s3Client.send(headCommand);

            return {
              id,
              name: headResponse.Metadata?.['project-name'] || id,
              path: item.Key!,
              size: item.Size || 0,
              lastModified: item.LastModified?.getTime() || Date.now(),
              provider: 's3' as CloudProvider,
              syncStatus: 'synced' as const,
              localCached: false,
            };
          } catch {
            // Fallback if head request fails
            return {
              id,
              name: id,
              path: item.Key!,
              size: item.Size || 0,
              lastModified: item.LastModified?.getTime() || Date.now(),
              provider: 's3' as CloudProvider,
              syncStatus: 'synced' as const,
              localCached: false,
            };
          }
        })
      );

      console.log(`✅ Found ${projects.length} projects in S3`);
      return projects;
    } catch (error) {
      console.error('❌ Failed to list S3 projects:', error);
      return [];
    }
  }

  /**
   * AWS S3 Delete
   */
  private async deleteFromS3(projectId: string): Promise<void> {
    console.log('Deleting from S3...');

    if (!this.config?.credentials) {
      throw new Error('S3 credentials not configured');
    }

    const { accessKeyId, secretAccessKey, region, bucket } = this.config.credentials;

    if (!accessKeyId || !secretAccessKey || !region || !bucket) {
      throw new Error('Missing S3 credentials');
    }

    try {
      const s3Client = new S3Client({
        region,
        credentials: {
          accessKeyId,
          secretAccessKey,
        },
      });

      const projectKey = `projects/${projectId}.json`;

      // Delete project file
      const deleteCommand = new DeleteObjectCommand({
        Bucket: bucket,
        Key: projectKey,
      });

      await s3Client.send(deleteCommand);
      console.log(`✅ Deleted project ${projectId} from S3`);

      // TODO: Also delete associated audio files in projects/${projectId}/audio/
      // This would require listing and deleting all objects with that prefix
    } catch (error) {
      console.error('❌ S3 delete failed:', error);
      throw new Error(`S3 delete failed: ${error}`);
    }
  }

  /**
   * Google Drive Upload (resumable for large files)
   */
  private async uploadToGoogleDrive(
    projectId: string,
    projectName: string,
    projectData: Blob,
    audioFiles: { name: string; blob: Blob }[]
  ): Promise<void> {
    console.log('Uploading to Google Drive...');

    if (!this.config?.credentials?.accessToken) {
      throw new Error('Google Drive not authenticated');
    }

    try {
      const API_BASE = 'https://www.googleapis.com/drive/v3';
      const UPLOAD_API_BASE = 'https://www.googleapis.com/upload/drive/v3';

      // Step 1: Initiate resumable upload
      const initResponse = await fetch(`${UPLOAD_API_BASE}/files?uploadType=resumable`, {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${this.config.credentials.accessToken}`,
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          name: `${projectName}.json`,
          mimeType: 'application/json',
          properties: {
            projectId,
            projectName,
            lastModified: Date.now().toString(),
            version: '1.0',
          },
        }),
      });

      if (!initResponse.ok) {
        throw new Error(`Failed to initiate upload: ${await initResponse.text()}`);
      }

      const resumableUri = initResponse.headers.get('Location');
      if (!resumableUri) {
        throw new Error('No resumable URI returned');
      }

      // Step 2: Upload the file data
      const arrayBuffer = await projectData.arrayBuffer();
      const uploadResponse = await fetch(resumableUri, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: arrayBuffer,
      });

      if (!uploadResponse.ok) {
        throw new Error(`Upload failed: ${await uploadResponse.text()}`);
      }

      const result = await uploadResponse.json();
      console.log('✅ Google Drive upload complete:', result);

      // Upload audio files (if any)
      for (const audioFile of audioFiles) {
        const audioInitResponse = await fetch(`${UPLOAD_API_BASE}/files?uploadType=resumable`, {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${this.config.credentials.accessToken}`,
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            name: audioFile.name,
            mimeType: 'audio/wav',
            parents: [result.id], // Store in same folder as project
          }),
        });

        const audioUri = audioInitResponse.headers.get('Location');
        if (audioUri) {
          const audioBuffer = await audioFile.blob.arrayBuffer();
          await fetch(audioUri, {
            method: 'PUT',
            headers: {
              'Content-Type': 'audio/wav',
            },
            body: audioBuffer,
          });
          console.log(`✅ Uploaded audio file: ${audioFile.name}`);
        }
      }

      this.emitProgress({
        projectId,
        fileName: projectName,
        loaded: projectData.size,
        total: projectData.size,
        percentage: 100,
        status: 'completed',
      });

      console.log(`✅ Project "${projectName}" uploaded to Google Drive successfully`);
    } catch (error) {
      console.error('❌ Google Drive upload failed:', error);
      this.emitProgress({
        projectId,
        fileName: projectName,
        loaded: 0,
        total: projectData.size,
        percentage: 0,
        status: 'error',
      });
      throw new Error(`Google Drive upload failed: ${error}`);
    }
  }

  /**
   * Google Drive Download
   */
  private async downloadFromGoogleDrive(projectId: string): Promise<{ data: Blob; audioFiles: { name: string; blob: Blob }[] }> {
    console.log('Downloading from Google Drive...');

    if (!this.config?.credentials?.accessToken) {
      throw new Error('Google Drive not authenticated');
    }

    try {
      const API_BASE = 'https://www.googleapis.com/drive/v3';

      // Download project file
      console.log(`Downloading project from Google Drive: ${projectId}`);

      const response = await fetch(`${API_BASE}/files/${projectId}?alt=media`, {
        headers: {
          Authorization: `Bearer ${this.config.credentials.accessToken}`,
        },
      });

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`Download failed: ${errorText}`);
      }

      const projectData = await response.blob();

      console.log('✅ Project downloaded from Google Drive');

      // Download associated audio files (if any)
      // You could search for files in the same folder or with specific metadata
      const audioFiles: { name: string; blob: Blob }[] = [];

      return {
        data: projectData,
        audioFiles,
      };
    } catch (error) {
      console.error('❌ Google Drive download failed:', error);
      throw new Error(`Google Drive download failed: ${error}`);
    }
  }

  /**
   * Google Drive List Projects
   */
  private async listGoogleDriveProjects(): Promise<CloudProject[]> {
    console.log('Listing Google Drive projects...');

    if (!this.config?.credentials?.accessToken) {
      return [];
    }

    try {
      const API_BASE = 'https://www.googleapis.com/drive/v3';

      // Search for JSON files (DAW projects)
      const response = await fetch(
        `${API_BASE}/files?q=mimeType='application/json' and name contains '.json'&fields=files(id,name,size,modifiedTime,properties)`,
        {
          headers: {
            'Authorization': `Bearer ${this.config.credentials.accessToken}`,
          },
        }
      );

      if (!response.ok) {
        throw new Error(`Failed to list projects: ${await response.text()}`);
      }

      const data = await response.json();

      const projects: CloudProject[] = (data.files || []).map((file: any) => ({
        id: file.properties?.projectId || file.id,
        name: file.properties?.projectName || file.name.replace('.json', ''),
        path: file.id,
        size: parseInt(file.size) || 0,
        lastModified: new Date(file.modifiedTime).getTime(),
        provider: 'google-drive' as CloudProvider,
        syncStatus: 'synced' as const,
        localCached: false,
      }));

      console.log(`✅ Found ${projects.length} projects in Google Drive`);
      return projects;
    } catch (error) {
      console.error('❌ Failed to list Google Drive projects:', error);
      return [];
    }
  }

  /**
   * Google Drive Delete
   */
  private async deleteFromGoogleDrive(projectId: string): Promise<void> {
    console.log('Deleting from Google Drive...');

    if (!this.config?.credentials?.accessToken) {
      throw new Error('Google Drive not authenticated');
    }

    try {
      const API_BASE = 'https://www.googleapis.com/drive/v3';

      const response = await fetch(`${API_BASE}/files/${projectId}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${this.config.credentials.accessToken}`,
        },
      });

      if (!response.ok && response.status !== 204) {
        throw new Error(`Delete failed: ${await response.text()}`);
      }

      console.log(`✅ Deleted project ${projectId} from Google Drive`);
    } catch (error) {
      console.error('❌ Google Drive delete failed:', error);
      throw new Error(`Google Drive delete failed: ${error}`);
    }
  }

  /**
   * Merge local and cloud project lists
   */
  private mergeProjectLists(
    local: CloudProject[],
    cloud: CloudProject[]
  ): CloudProject[] {
    const merged = new Map<string, CloudProject>();

    // Add cloud projects
    cloud.forEach((p) => merged.set(p.id, p));

    // Merge local projects
    local.forEach((p) => {
      if (merged.has(p.id)) {
        // Mark as cached
        merged.get(p.id)!.localCached = true;
      } else {
        // Local-only project
        merged.set(p.id, p);
      }
    });

    return Array.from(merged.values());
  }

  /**
   * Emit upload progress
   */
  private emitProgress(progress: UploadProgress): void {
    this.uploadListeners.forEach((listener) => listener(progress));
  }

  /**
   * Listen for upload progress
   */
  onUploadProgress(listener: (progress: UploadProgress) => void): void {
    this.uploadListeners.push(listener);
  }

  /**
   * Remove upload progress listener
   */
  offUploadProgress(listener: (progress: UploadProgress) => void): void {
    this.uploadListeners = this.uploadListeners.filter((l) => l !== listener);
  }
}

export const cloudStorage = CloudStorageService.getInstance();
