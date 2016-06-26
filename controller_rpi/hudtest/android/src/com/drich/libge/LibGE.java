package com.drich.libge;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.util.Locale;
import java.util.List;
import java.util.LinkedList;
import android.app.NativeActivity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.accounts.AccountManager;
import android.accounts.Account;
import android.opengl.GLSurfaceView;
import android.view.Display;
import android.view.View;
import android.view.Window;
import android.view.SurfaceView;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import android.view.KeyEvent;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Point;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.EditText;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.InterstitialAd;
import com.google.android.gms.ads.AdListener;


public class LibGE extends NativeActivity
{
	static {
		System.loadLibrary("gammaengine");
	}

	public static final boolean mFullscreen = true;

	public static native void setHasSurface( int hasSurface );
	public static native void setSurface( Surface surface, int x, int y );

	public static native boolean adsVisible();
// 	public static native int adsType();
	public static native void setAdsVisible(boolean visible);
// 	public static native boolean imeVisible();
// 	public static native void setImeState(int state, String ret);
	public static native boolean exiting();
	public static native void backButtonPressed();

	public static native void AndroidInit( String locale, String username, String email );

	private GLSurfaceView glSurfaceView;
	private SurfaceView surfaceView;
	private Surface surface;

	private InterstitialAd mInterstitialAd;
	public boolean mAdsVisible = false;
	public int mAdsType = 0;

	public boolean mImeVisible = false;
	public AlertDialog imeDialog;
	public EditText textEdit;

	public void onCreate(Bundle savedInstanceState)
	{
		AndroidInit( getLocale(), getUserName(), getUserEmail() );

		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.onCreate(savedInstanceState);

		if ( mFullscreen ) {
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		} else {
			setHasSurface(1);
			surfaceView = new SurfaceView(this);

			Display display = getWindow().getWindowManager().getDefaultDisplay();
			Point size = new Point();
			display.getSize(size);
			int width = size.x;
			int height = size.y;
			if ( width > 720 ) {
				surfaceView.getHolder().setFixedSize( 720, 720 * height / width );
			}

			surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
				@Override
				public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
					surface = holder.getSurface();
					android.util.Log.i( "GE", "SurfaceView size : " + surfaceView.getWidth() + " x " + surfaceView.getHeight() );
					int[] loc = new int[2];
					surfaceView.getLocationOnScreen(loc);
					setSurface(surface, loc[0], loc[1]);
				}
				@Override
				public void surfaceCreated(SurfaceHolder holder) {
				}
				@Override
				public void surfaceDestroyed(SurfaceHolder holder) {}
			});

			getWindow().setContentView(surfaceView);
		}

		mInterstitialAd = new InterstitialAd(this);
		mInterstitialAd.setAdUnitId("ca-app-pub-2014753020255514/8946404983");
		final AdRequest.Builder adRequestBuilder = new AdRequest.Builder();

		adRequestBuilder.addTestDevice(AdRequest.DEVICE_ID_EMULATOR);
		adRequestBuilder.addTestDevice("F5EBFA82B1A4EA644145159C1F2D9AE4");
		adRequestBuilder.addTestDevice("08A38690FE82E8FDD1121F3F69D4EFD1");
		adRequestBuilder.addTestDevice("14C65F316109EA8D02603F43EC296535");
		adRequestBuilder.addTestDevice("CA4990E11120E7E222988478E7CEB5F5");

		AdRequest adRequest = adRequestBuilder.build();
		mInterstitialAd.loadAd(adRequest);

		mInterstitialAd.setAdListener(new AdListener() {
			@Override
			public void onAdClosed() {
				AdRequest adRequest = adRequestBuilder.build();
				mInterstitialAd.loadAd(adRequest);
				mAdsVisible = false;
				setAdsVisible(false);
			}
		});
/*
		final AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(getWindow().getContext())
			.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					setImeState(1, textEdit.getText().toString());
					mImeVisible = false;
					
				}
			})
			.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					setImeState(2, "");
					mImeVisible = false;
				}
			});
*/
		final Handler handler = new Handler();
		handler.postDelayed(new Runnable() {
			@Override
			public void run() {
				if(adsVisible() && !mAdsVisible){
					if(mInterstitialAd.isLoaded()){
						android.util.Log.i("GE", "Ad loaded !");
						mAdsVisible = true;
						mInterstitialAd.show();
					}else{
						android.util.Log.i("GE", "Ad NOT loaded !");
						mAdsVisible = false;
						setAdsVisible(false);
						AdRequest adRequest = adRequestBuilder.build();
						mInterstitialAd.loadAd(adRequest);
					}
				}
				if(!adsVisible() && mAdsVisible){
					mAdsVisible = false;
				}
/*
				if(imeVisible() && !mImeVisible){
					mImeVisible = true;
					textEdit = new EditText(getWindow().getContext());
					textEdit.setLines(1);
					dialogBuilder.setView(textEdit);
					dialogBuilder.setTitle("input text");
					dialogBuilder.setMessage("Enter username");
					imeDialog = dialogBuilder.show();
				}
				if(mImeVisible && !imeDialog.isShowing()){
					setImeState(2, "");
					mImeVisible = false;
				}
*/
				if ( exiting() ) {
					android.util.Log.i("GE", "Exiting !\n");
					System.exit(1);
				}
				handler.postDelayed(this, 250);
			}
		}, 250);
	}

	public String getLocale() {
		Locale current = getResources().getConfiguration().locale;
		if ( current == null ) {
			return "";
		}
		return current.getLanguage();
	}

	public String getUserName() {
		String email = getUserEmail();
		String[] parts = email.split( "@" );
		if ( parts.length > 0 && parts[0] != null ) {
			return parts[0];
		}
		return "";
	}

	public String getUserEmail() {
		AccountManager manager = AccountManager.get(this);
		Account[] accounts = manager.getAccountsByType("com.google");
		List<String> possibleEmails = new LinkedList<String>();

		for ( Account account : accounts ) {
			possibleEmails.add( account.name );
		}

		if ( !possibleEmails.isEmpty() && possibleEmails.get(0) != null ) {
			return possibleEmails.get(0);
		}
		return "";
	}

	@Override
	public void onBackPressed()
	{
// 		backButtonPressed();
	}

	@Override
	public boolean onKeyDown( int keyCode, KeyEvent event )
	{
		return super.onKeyDown( keyCode, event );
	}

	@Override
	public void onStart()
	{
		super.onStart();
	}
}
