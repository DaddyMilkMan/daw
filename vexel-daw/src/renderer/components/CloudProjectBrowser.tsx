/**
 * Cloud Project Browser
 * Browse, open, and manage cloud-stored projects
 */

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  X,
  Cloud,
  Download,
  Upload,
  Trash2,
  RefreshCw,
  HardDrive,
  CheckCircle,
  AlertCircle,
  Loader,
  FolderOpen,
} from 'lucide-react';
import { Button } from './ui/button';
import { cloudStorage } from '../services/cloudStorage';
import { CloudProject, CloudProvider } from '../services/cloudStorage';

interface CloudProjectBrowserProps {
  open: boolean;
  onClose: () => void;
  onSelectProject?: (project: CloudProject) => void;
}

const PROVIDER_ICONS = {
  s3: Cloud,
  'google-drive': Cloud,
  local: HardDrive,
};

const STATUS_ICONS = {
  synced: CheckCircle,
  uploading: Upload,
  downloading: Download,
  conflict: AlertCircle,
  error: AlertCircle,
};

const STATUS_COLORS = {
  synced: 'text-green-500',
  uploading: 'text-blue-500 animate-pulse',
  downloading: 'text-blue-500 animate-pulse',
  conflict: 'text-yellow-500',
  error: 'text-red-500',
};

export default function CloudProjectBrowser({
  open,
  onClose,
  onSelectProject,
}: CloudProjectBrowserProps) {
  const [projects, setProjects] = useState<CloudProject[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [filterProvider, setFilterProvider] = useState<CloudProvider | 'all'>('all');
  const [sortBy, setSortBy] = useState<'name' | 'date' | 'size'>('date');

  useEffect(() => {
    if (open) {
      loadProjects();
    }
  }, [open]);

  const loadProjects = async () => {
    setIsLoading(true);
    setError(null);

    try {
      const projectList = await cloudStorage.listProjects();
      setProjects(projectList);
    } catch (err) {
      console.error('Failed to load projects:', err);
      setError(err instanceof Error ? err.message : 'Failed to load projects');
    } finally {
      setIsLoading(false);
    }
  };

  const handleDeleteProject = async (projectId: string) => {
    if (!confirm('Are you sure you want to delete this project?')) {
      return;
    }

    try {
      await cloudStorage.deleteProject(projectId);
      setProjects(projects.filter((p) => p.id !== projectId));
    } catch (err) {
      console.error('Failed to delete project:', err);
      alert('Failed to delete project');
    }
  };

  const handleDownloadProject = async (projectId: string) => {
    try {
      await cloudStorage.downloadProject(projectId);
      await loadProjects(); // Refresh to show cached status
      alert('Project downloaded successfully!');
    } catch (err) {
      console.error('Failed to download project:', err);
      alert('Failed to download project');
    }
  };

  const handleSelectProject = (project: CloudProject) => {
    if (onSelectProject) {
      onSelectProject(project);
      onClose();
    }
  };

  // Filter and sort projects
  let filteredProjects = projects;

  if (filterProvider !== 'all') {
    filteredProjects = filteredProjects.filter((p) => p.provider === filterProvider);
  }

  filteredProjects = [...filteredProjects].sort((a, b) => {
    switch (sortBy) {
      case 'name':
        return a.name.localeCompare(b.name);
      case 'date':
        return b.lastModified - a.lastModified;
      case 'size':
        return b.size - a.size;
      default:
        return 0;
    }
  });

  // Format file size
  const formatSize = (bytes: number): string => {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
  };

  // Format date
  const formatDate = (timestamp: number): string => {
    const date = new Date(timestamp);
    const now = new Date();
    const diff = now.getTime() - date.getTime();
    const days = Math.floor(diff / (1000 * 60 * 60 * 24));

    if (days === 0) return 'Today';
    if (days === 1) return 'Yesterday';
    if (days < 7) return `${days} days ago`;
    return date.toLocaleDateString();
  };

  if (!open) return null;

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm z-50 flex items-center justify-center p-4"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.95, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.95, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg shadow-2xl w-full max-w-5xl max-h-[85vh] overflow-hidden flex flex-col"
        >
          {/* Header */}
          <div className="flex items-center justify-between p-6 border-b border-border">
            <div>
              <h2 className="text-2xl font-bold text-foreground flex items-center gap-2">
                <Cloud className="h-6 w-6 text-primary" />
                Cloud Projects
              </h2>
              <p className="text-sm text-muted-foreground mt-1">
                Browse and manage your cloud-stored projects
              </p>
            </div>
            <div className="flex items-center gap-2">
              <Button
                size="icon"
                variant="ghost"
                onClick={loadProjects}
                disabled={isLoading}
              >
                <RefreshCw className={`h-5 w-5 ${isLoading ? 'animate-spin' : ''}`} />
              </Button>
              <Button size="icon" variant="ghost" onClick={onClose}>
                <X className="h-5 w-5" />
              </Button>
            </div>
          </div>

          {/* Filters */}
          <div className="p-6 border-b border-border bg-muted/30">
            <div className="flex items-center gap-4">
              <div className="flex items-center gap-2">
                <span className="text-sm text-muted-foreground">Provider:</span>
                <div className="flex gap-2">
                  <Button
                    size="sm"
                    variant={filterProvider === 'all' ? 'default' : 'outline'}
                    onClick={() => setFilterProvider('all')}
                  >
                    All
                  </Button>
                  <Button
                    size="sm"
                    variant={filterProvider === 's3' ? 'default' : 'outline'}
                    onClick={() => setFilterProvider('s3')}
                  >
                    S3
                  </Button>
                  <Button
                    size="sm"
                    variant={filterProvider === 'google-drive' ? 'default' : 'outline'}
                    onClick={() => setFilterProvider('google-drive')}
                  >
                    Google Drive
                  </Button>
                  <Button
                    size="sm"
                    variant={filterProvider === 'local' ? 'default' : 'outline'}
                    onClick={() => setFilterProvider('local')}
                  >
                    Local
                  </Button>
                </div>
              </div>

              <div className="flex-1" />

              <div className="flex items-center gap-2">
                <span className="text-sm text-muted-foreground">Sort by:</span>
                <select
                  value={sortBy}
                  onChange={(e) => setSortBy(e.target.value as any)}
                  className="px-3 py-1.5 bg-background border border-border rounded-md text-sm focus:outline-none focus:ring-2 focus:ring-primary"
                >
                  <option value="date">Date Modified</option>
                  <option value="name">Name</option>
                  <option value="size">Size</option>
                </select>
              </div>
            </div>
          </div>

          {/* Project List */}
          <div className="flex-1 overflow-y-auto p-6">
            {error && (
              <div className="bg-red-500/10 border border-red-500/30 rounded-lg p-4 mb-4">
                <p className="text-red-500 text-sm">{error}</p>
              </div>
            )}

            {isLoading ? (
              <div className="flex items-center justify-center py-12">
                <Loader className="h-8 w-8 animate-spin text-primary" />
              </div>
            ) : filteredProjects.length === 0 ? (
              <div className="text-center py-12">
                <Cloud className="h-16 w-16 text-muted-foreground mx-auto mb-4" />
                <p className="text-muted-foreground text-lg">No projects found</p>
                <p className="text-sm text-muted-foreground mt-2">
                  Upload a project to get started
                </p>
              </div>
            ) : (
              <div className="space-y-3">
                {filteredProjects.map((project) => {
                  const ProviderIcon = PROVIDER_ICONS[project.provider];
                  const StatusIcon = STATUS_ICONS[project.syncStatus];
                  const statusColor = STATUS_COLORS[project.syncStatus];

                  return (
                    <motion.div
                      key={project.id}
                      layout
                      initial={{ opacity: 0, y: 20 }}
                      animate={{ opacity: 1, y: 0 }}
                      className="bg-muted/50 border border-border rounded-lg p-4 hover:bg-muted/70 transition-colors cursor-pointer"
                      onClick={() => handleSelectProject(project)}
                    >
                      <div className="flex items-center gap-4">
                        {/* Icon */}
                        <div className="flex-shrink-0">
                          <FolderOpen className="h-10 w-10 text-primary" />
                        </div>

                        {/* Info */}
                        <div className="flex-1 min-w-0">
                          <h3 className="font-semibold text-foreground truncate">
                            {project.name}
                          </h3>
                          <div className="flex items-center gap-3 mt-1">
                            <div className="flex items-center gap-1 text-xs text-muted-foreground">
                              <ProviderIcon className="h-3 w-3" />
                              <span className="capitalize">{project.provider.replace('-', ' ')}</span>
                            </div>
                            <span className="text-xs text-muted-foreground">
                              {formatSize(project.size)}
                            </span>
                            <span className="text-xs text-muted-foreground">
                              {formatDate(project.lastModified)}
                            </span>
                            {project.localCached && (
                              <span className="text-xs bg-green-500/20 text-green-500 px-2 py-0.5 rounded">
                                Cached
                              </span>
                            )}
                          </div>
                        </div>

                        {/* Status */}
                        <div className="flex items-center gap-2">
                          <StatusIcon className={`h-5 w-5 ${statusColor}`} />
                          <span className={`text-xs ${statusColor}`}>
                            {project.syncStatus}
                          </span>
                        </div>

                        {/* Actions */}
                        <div className="flex items-center gap-2">
                          {!project.localCached && (
                            <Button
                              size="icon"
                              variant="ghost"
                              onClick={(e) => {
                                e.stopPropagation();
                                handleDownloadProject(project.id);
                              }}
                              title="Download to cache"
                            >
                              <Download className="h-4 w-4" />
                            </Button>
                          )}
                          <Button
                            size="icon"
                            variant="ghost"
                            onClick={(e) => {
                              e.stopPropagation();
                              handleDeleteProject(project.id);
                            }}
                            className="text-red-500 hover:text-red-600 hover:bg-red-500/10"
                            title="Delete project"
                          >
                            <Trash2 className="h-4 w-4" />
                          </Button>
                        </div>
                      </div>
                    </motion.div>
                  );
                })}
              </div>
            )}
          </div>

          {/* Footer Stats */}
          <div className="p-6 border-t border-border bg-muted/30">
            <div className="flex items-center justify-between text-sm">
              <div className="flex items-center gap-6">
                <div>
                  <span className="text-muted-foreground">Total Projects: </span>
                  <span className="font-semibold text-foreground">{projects.length}</span>
                </div>
                <div>
                  <span className="text-muted-foreground">Cached: </span>
                  <span className="font-semibold text-foreground">
                    {projects.filter((p) => p.localCached).length}
                  </span>
                </div>
                <div>
                  <span className="text-muted-foreground">Total Size: </span>
                  <span className="font-semibold text-foreground">
                    {formatSize(projects.reduce((sum, p) => sum + p.size, 0))}
                  </span>
                </div>
              </div>

              <div className="text-xs text-muted-foreground">
                Click a project to open it
              </div>
            </div>
          </div>
        </motion.div>
      </motion.div>
    </AnimatePresence>
  );
}
