# Cloud Storage Integration

Vexel DAW supports multiple cloud storage providers for project synchronization.

## Supported Providers

### Built-in (Always Available)
- **AWS S3** - Full support with multipart uploads
- **Google Drive** - Full support with resumable uploads
- **IndexedDB** - Local browser storage (fallback)

### Optional (Requires Additional Installation)
- **MEGA.nz** - Requires `megajs` package
- **pCloud** - Requires `pcloud-sdk-js` package

## Installing Optional Cloud Providers

The MEGA and pCloud connectors are optional and not installed by default to keep the bundle size small.

### To Enable MEGA.nz Support

```bash
npm install megajs
```

### To Enable pCloud Support

```bash
npm install pcloud-sdk-js
```

## TypeScript Compilation

Ambient module declarations are provided in `src/types/optional-modules.d.ts` to allow TypeScript compilation even when optional packages are not installed. Runtime fallback code will gracefully handle missing dependencies.

## How It Works

1. **With Optional Package Installed**: Full functionality enabled
2. **Without Optional Package**:
   - TypeScript compiles successfully (ambient declarations)
   - Runtime shows warning: "Package not installed"
   - Connector returns error status indicating SDK is missing

## Adding New Cloud Providers

To add a new optional cloud provider:

1. Create connector class in `src/renderer/services/connectors/`
2. Implement the `ICloudConnector` interface
3. Add ambient module declaration in `src/types/optional-modules.d.ts`
4. Add to `optionalDependencies` in `package.json`
5. Document installation instructions here

## Configuration

See `src/renderer/types/connectors.ts` for connector configuration interface.

Example configuration:

```typescript
const config: ConnectorConfig = {
  id: 'mega-main',
  provider: 'mega',
  credentials: {
    email: 'user@example.com',
    password: 'secret'
  }
};
```
