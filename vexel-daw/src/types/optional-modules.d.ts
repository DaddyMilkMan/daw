/**
 * Ambient module declarations for optional cloud storage SDK dependencies
 *
 * These packages are NOT included in package.json by default.
 * They are optional dependencies that users can install if needed:
 *
 * - npm install megajs
 * - npm install pcloud-sdk-js
 *
 * These declarations allow TypeScript to compile without errors
 * even when the packages are not installed. Runtime fallback code
 * will handle missing dependencies gracefully.
 */

/**
 * MEGA.nz SDK
 * https://www.npmjs.com/package/megajs
 */
declare module 'megajs' {
  export interface StorageOptions {
    email: string;
    password: string;
    keepalive?: boolean;
    autologin?: boolean;
  }

  export interface FileOptions {
    name: string;
    attributes?: any;
    size?: number;
  }

  export class Storage {
    constructor(options: StorageOptions, callback?: (error: Error | null) => void);

    ready(callback: (error: Error | null) => void): void;
    login(options: { email: string; password: string }, callback?: (error: Error | null) => void): void;

    upload(options: FileOptions | string, buffer?: Buffer, callback?: (error: Error | null, file: any) => void): any;
    download(fileOrLink: any, callback?: (error: Error | null, data: Buffer) => void): any;

    mkdir(options: string | { name: string }, callback?: (error: Error | null, folder: any) => void): any;
    root: any;
    files: any[];

    on(event: string, callback: Function): void;
    close(): void;
  }

  export class File {
    name: string;
    size: number;
    key: string;
    downloadId: string;

    download(callback?: (error: Error | null, data: Buffer) => void): any;
    delete(callback?: (error: Error | null) => void): void;
  }

  export default Storage;
}

/**
 * pCloud SDK
 * https://www.npmjs.com/package/pcloud-sdk-js
 */
declare module 'pcloud-sdk-js' {
  export interface PCloudConfig {
    client_id?: string;
    redirect_uri?: string;
    auth_token?: string;
  }

  export interface UploadOptions {
    files: File[] | FileList;
    folderId?: number;
    onProgress?: (progress: number) => void;
  }

  export interface DownloadOptions {
    fileid: number;
  }

  export interface FileMetadata {
    fileid: number;
    name: string;
    size: number;
    isfolder: boolean;
    parentfolderid: number;
    created: string;
    modified: string;
    path?: string;
  }

  export interface FolderMetadata {
    folderid: number;
    name: string;
    isfolder: true;
    parentfolderid: number;
    created: string;
    modified: string;
    contents?: (FileMetadata | FolderMetadata)[];
  }

  export interface UserInfo {
    userid: number;
    email: string;
    quota: number;
    usedquota: number;
  }

  export default class pCloud {
    constructor(config?: PCloudConfig);

    static createClient(authToken: string): pCloud;

    setAuthToken(token: string): void;
    getAuthToken(): string | null;

    // Authentication
    authorize(): Promise<string>;
    logout(): Promise<void>;

    // File operations
    uploadFile(options: UploadOptions): Promise<FileMetadata[]>;
    downloadFile(options: DownloadOptions): Promise<Blob>;
    deleteFile(fileid: number): Promise<void>;

    // Folder operations
    createFolder(name: string, parentFolderId?: number): Promise<FolderMetadata>;
    listFolder(folderId?: number): Promise<FolderMetadata>;
    deleteFolder(folderId: number): Promise<void>;

    // User info
    getUserInfo(): Promise<UserInfo>;

    // Search
    searchFiles(query: string): Promise<FileMetadata[]>;
  }
}
